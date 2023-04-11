#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"
#include "relay.h"
#include "tmr.h"
#include "rema.h"

#define MOT_PAP_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define MOT_PAP_HELPER_TASK_PRIORITY ( configMAX_PRIORITIES - 3)
QueueHandle_t isr_helper_task_queue = NULL;

extern int count_a;

mot_pap::mot_pap(const char *name, class tmr t) :
        name(name), type(TYPE_STOP), last_dir(DIRECTION_CW), half_pulses(0), offset(
                0), tmr(t) {
}

void mot_pap::task() {
    struct mot_pap_msg *msg_rcv;

    if (xQueueReceive(queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
        lDebug(Info, "%s: command received", name);

        stalled = false; // If a new command was received, assume we are not stalled
        stalled_counter = 0;
        already_there = false;

        using namespace std::chrono_literals;
        step_time = 100ms;

        switch (msg_rcv->type) {
        case mot_pap::TYPE_FREE_RUNNING:
            move_free_run(msg_rcv->free_run_direction,
                    msg_rcv->free_run_speed);
            break;

        case mot_pap::TYPE_CLOSED_LOOP:
            move_closed_loop(msg_rcv->closed_loop_setpoint * inches_to_counts_factor);
            break;

        default:
            stop();
            break;
        }

        vPortFree (msg_rcv);
        msg_rcv = NULL;
    }
}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	MOT_PAP_DIRECTION_CW if error is positive
 * @returns	MOT_PAP_DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int32_t error) const {
    return error < 0 ? direction::DIRECTION_CCW : DIRECTION_CW;
}

/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either MOT_PAP_DIRECTION_CW or MOT_PAP_DIRECTION_CCW
 * @param 	speed		: integer from 0 to 8
 * @returns	nothing
 */
void mot_pap::move_free_run(enum direction direction, int speed) {
    if (speed < MOT_PAP_MAX_FREQ) {
        stalled = false; // If a new command was received, assume we are not stalled
        stalled_counter = 0;
        already_there = false;

        if ((dir != direction) && (type != TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = TYPE_FREE_RUNNING;
        dir = direction;
        gpios.direction.set_pin_state(dir);
        requested_freq = speed;

        tmr.stop();
        tmr.set_freq(requested_freq);
        tmr.start();
        lDebug(Info, "%s: FREE RUN, speed: %li, direction: %s", name,
                requested_freq, dir == DIRECTION_CW ? "CW" : "CCW");
    } else {
        lDebug(Warn, "%s: chosen speed out of bounds %i", name, speed);
    }
}

void mot_pap::move_closed_loop(int setpoint) {
    int32_t error;
    bool already_there;
    enum direction new_dir;
    stalled = false; // If a new command was received, assume we are not stalled
    stalled_counter = 0;

    pos_cmd = setpoint;
    lDebug(Info, "%s: CLOSED_LOOP posCmd: %i posAct: %i", name, pos_cmd,
            pos_act);

    //calculate position error
    error = pos_cmd - pos_act;
    already_there = (abs(error) < MOT_PAP_POS_THRESHOLD);

    if (already_there) {
        tmr.stop();
        lDebug(Info, "%s: already there", name);
    } else {
        kp.restart(pos_act);

        int out = kp.run(pos_cmd, pos_act);
        new_dir = direction_calculate(out);
        if ((dir != new_dir) && (type != TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = TYPE_CLOSED_LOOP;
        dir = new_dir;

        gpios.direction.set_pin_state(dir);
        requested_freq = abs(out);
        tmr.stop();
        tmr.set_freq(requested_freq);
        tmr.start();
    }
}

/**
 * @brief	if there is a movement in process, stops it
 * @param 	me	: struct mot_pap pointer
 * @returns	nothing
 */
void mot_pap::stop() {
    type = TYPE_STOP;
    tmr.stop();
    lDebug(Info, "%s: STOP", name);
}

/**
 * @brief   supervise motor movement for stall or position reached in closed loop
 * @returns nothing
 * @note    to be called by the deferred interrupt task handler
 */
void mot_pap::supervise() {

    while (true) {

        if (xSemaphoreTake(supervisor_semaphore,
                portMAX_DELAY) == pdPASS) {
            if (rema::stall_control_get()) {
                if (abs((int) (pos_act - last_pos)) < MOT_PAP_STALL_THRESHOLD) {

                    stalled_counter++;
                    if (stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
                        stalled = true;
                        tmr.stop();
                        lDebug(Warn, "%s: stalled", name);
                        rema::control_enabled_set(false);
                        goto end;
                    }
                } else {
                    stalled_counter = 0;
                }
            }

            if (already_there) {
                lDebug(Info, "%s: position reached", name);
                goto end;
            }

            if (type == TYPE_CLOSED_LOOP) {
                int out = kp.run(pos_cmd, pos_act);
                lDebug(Info, "Control output = %i: ", out);

                enum direction dir = direction_calculate(out);
                if ((this->dir != dir) && (type != TYPE_STOP)) {
                    tmr.stop();
                    vTaskDelay(
                            pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
                }
                this->dir = dir;
                gpios.direction.set_pin_state(dir);
                requested_freq = abs(out);
                tmr.stop();
                tmr.set_freq(requested_freq);
                tmr.start();
            }

        }
    }
    end: ;
}

/**
 * @brief 	function called by the timer ISR to generate the output pulses
 */
void mot_pap::isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t ticks_now = xTaskGetTickCount();

    int error;
    if (type == TYPE_CLOSED_LOOP) {
        error = pos_cmd - pos_act;
        already_there = (abs((int) error) < 2);
    }

    if (already_there) {
        type = TYPE_STOP;
        tmr.stop();
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        goto cont;
    }

    ++half_pulses;

    gpios.step.toggle();

    if ((ticks_now - ticks_last_time) > pdMS_TO_TICKS(step_time.count())) {

        ticks_last_time = ticks_now;
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    cont: last_pos = pos_act;
}

/**
 * @brief 	updates the current position from RDC
 */
void mot_pap::update_position() {
    if (dir == DIRECTION_CW) {
        pos_act ++;
    } else {
        pos_act --;
    }
}

JSON_Value* mot_pap::json() const {
    JSON_Value *ans = json_value_init_object();
    json_object_set_number(json_value_get_object(ans), "posCmd", pos_cmd);
    json_object_set_number(json_value_get_object(ans), "posAct", pos_act);
    json_object_set_boolean(json_value_get_object(ans), "stalled", stalled);
    json_object_set_number(json_value_get_object(ans), "offset", offset);
    return ans;
}
