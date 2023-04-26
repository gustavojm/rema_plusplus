#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "task.h"

#include "debug.h"
#include "rema.h"
#include "bresenham.h"

using namespace std::chrono_literals;

#define BRESENHAM_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )

extern mot_pap x_axis;
extern mot_pap y_axis;

void bresenham::task() {
    struct bresenham_msg *msg_rcv;

    if (xQueueReceive(queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
        lDebug(Info, "%s: command received", name);

        switch (msg_rcv->type) {
        case mot_pap::TYPE_BRESENHAM:
            move(msg_rcv->x_setpoint * x_axis.inches_to_counts_factor, msg_rcv->y_setpoint * y_axis.inches_to_counts_factor);
            break;

        default:
            stop();
            break;
        }

        vPortFree (msg_rcv);
        msg_rcv = NULL;
    }
}

void bresenham::move(int setpoint_x, int setpoint_y) {
    type = TYPE_BRESENHAM;
    already_there = false;
    x_axis.pos_cmd = setpoint_x;
    y_axis.pos_cmd = setpoint_y;

    int dx = x_axis.pos_cmd - x_axis.pos_act;
    int dy = y_axis.pos_cmd - y_axis.pos_act;

    x_axis.delta = abs(dx);
    y_axis.delta = abs(dy);

    x_axis.set_direction();
    y_axis.set_direction();

    error =  x_axis.delta - y_axis.delta;

    if (x_axis.delta > y_axis.delta) {
        leader_axis = &x_axis;
    } else {
        leader_axis = &y_axis;
    }

    if (x_axis.check_already_there() && y_axis.check_already_there()) {
        stop();
        lDebug(Info, "%s: already there", name);
    } else {
        kp.restart(leader_axis->pos_act);

        int out = kp.run(leader_axis->pos_cmd, leader_axis->pos_act);
        requested_freq = abs(out);
        tmr.change_freq(requested_freq);
    }
    lDebug(Info, "%s: MOVE, X: %i, Y: %i", name, setpoint_x, setpoint_y);
}

void bresenham::step() {
    int e2 = error << 1;
    if (e2 >= -y_axis.delta) {
        error -= y_axis.delta;
        if (!x_axis.check_already_there()) {
            x_axis.step();
        }
    }
    if (e2 <= x_axis.delta) {
        error += x_axis.delta;
        if (!y_axis.check_already_there()) {
            y_axis.step();
        }
    }
}

/**
 * @brief   supervise motor movement for stall or position reached in closed loop
 * @returns nothing
 * @note    to be called by the deferred interrupt task handler
 */
void bresenham::supervise() {
    if (xSemaphoreTake(supervisor_semaphore,
            portMAX_DELAY) == pdPASS) {
        if (already_there) {
            lDebug(Info, "%s: position reached", name);
            stop();
            goto end;
        }

        if (rema::stall_control_get()) {
            bool x_stalled = x_axis.check_for_stall();   // make sure both stall checks are executed;
            bool y_stalled = y_axis.check_for_stall();   // make sure both stall checks are executed;

            if (x_stalled || y_stalled) {
                stop();
                rema::control_enabled_set(false);
                goto end;
            }
        }

        if (type == TYPE_BRESENHAM) {
            x_axis.set_direction();     // if didn't stop for proximity to set point, avoid going to infinity
            y_axis.set_direction();     // keeps dancing around the setpoint...

            int out = kp.run(leader_axis->pos_cmd, leader_axis->pos_act);
            lDebug(Info, "Control output = %i: ", out);
            requested_freq = abs(out);
            tmr.change_freq(requested_freq);
        }

    }
    end: ;
}

/**if (x_axis->check_already_there()
 * @brief   function called by the timer ISR to generate the output pulses
 */
void bresenham::isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t ticks_now = xTaskGetTickCount();


    if (type == TYPE_BRESENHAM) {
        already_there = x_axis.check_already_there() && y_axis.check_already_there();
        if (already_there) {
            stop();
            xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            goto cont;
        }
    }

    step();

    if ((ticks_now - ticks_last_time) > pdMS_TO_TICKS(step_time.count())) {
        ticks_last_time = ticks_now;
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    cont: ;
}

/**
 * @brief   if there is a movement in process, stops it
 * @param   me  : struct mot_pap pointer
 * @returns nothing
 */
void bresenham::stop() {
    tmr.stop();
    type = TYPE_STOP;
}
