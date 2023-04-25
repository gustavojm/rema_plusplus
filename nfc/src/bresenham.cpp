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
    x_axis.pos_cmd = setpoint_x;
    y_axis.pos_cmd = setpoint_y;

    int dx = abs(x_axis.pos_cmd - x_axis.pos_act);
    int dy = abs(y_axis.pos_cmd - y_axis.pos_act);

    x_axis.delta = dx;
    y_axis.delta = dy;

    x_axis.set_direction(x_axis.direction_calculate(dx));
    y_axis.set_direction(y_axis.direction_calculate(dy));

    if (dx > dy) {
        leader_axis = &x_axis;
        follower_axis = &y_axis;
    } else {
        leader_axis = &y_axis;
        follower_axis = &x_axis;
    }

    if (leader_axis->check_already_there() && follower_axis->check_already_there()) {
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
    int over = leader_axis->delta >> 1;
    leader_axis->step();
    over += follower_axis->delta;
    if (over >= leader_axis->delta) {
        over -= leader_axis->delta;
        follower_axis->step();
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
        if (rema::stall_control_get()) {
            bool leader_stalled = leader_axis->check_for_stall();       // make sure both stall checks are executed;
            bool follower_stalled = follower_axis->check_for_stall();   // make sure both stall checks are executed;

            if (leader_stalled || follower_stalled) {
                stop();
                rema::control_enabled_set(false);
                goto end;
            }
        }

        if (leader_axis->check_already_there()
                || follower_axis->check_already_there()) {
            lDebug(Info, "%s: position reached", name);
            goto end;
        }

        if (type == TYPE_BRESENHAM) {
            int out = kp.run(leader_axis->pos_cmd, leader_axis->pos_act);
            lDebug(Info, "Control output = %i: ", out);
            requested_freq = abs(out);
            tmr.change_freq(requested_freq);
        }

    }
    end: ;
}

/**
 * @brief   function called by the timer ISR to generate the output pulses
 */
void bresenham::isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t ticks_now = xTaskGetTickCount();


    if (type == TYPE_BRESENHAM) {
        if (leader_axis->check_already_there() || follower_axis->check_already_there()) {
            tmr.stop();
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
    type = TYPE_STOP;
    tmr.stop();
    lDebug(Info, "%s: STOP", name);
}
