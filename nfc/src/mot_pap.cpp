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

// Frequencies expressed in Khz
const uint32_t mot_pap::free_run_freqs[] =
        { 0, 25, 25, 25, 50, 75, 75, 100, 125 };

#define MOT_PAP_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define MOT_PAP_HELPER_TASK_PRIORITY ( configMAX_PRIORITIES - 3)
QueueHandle_t isr_helper_task_queue = NULL;

extern int count_a;

mot_pap::mot_pap(const char *name, class tmr t) :
        name(name), type(MOT_PAP_TYPE_STOP), last_dir(MOT_PAP_DIRECTION_CW), half_pulses(
                0), offset(0), tmr(t) {
}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	MOT_PAP_DIRECTION_CW if error is positive
 * @returns	MOT_PAP_DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int32_t error) const {
    return error < 0 ? MOT_PAP_DIRECTION_CW : MOT_PAP_DIRECTION_CCW;
}

/**
 * @brief 	checks if the required FREE RUN speed is in the allowed range
 * @param 	speed : the requested speed
 * @returns	true if speed is in the allowed range
 */
bool mot_pap::free_run_speed_ok(uint32_t speed) const {
    return ((speed > 0) && (speed <= MOT_PAP_MAX_SPEED_FREE_RUN));
}

/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either MOT_PAP_DIRECTION_CW or MOT_PAP_DIRECTION_CCW
 * @param 	speed		: integer from 0 to 8
 * @returns	nothing
 */
void mot_pap::move_free_run(enum direction direction, uint32_t speed) {
    if (free_run_speed_ok(speed)) {
        stalled = false; // If a new command was received, assume we are not stalled
        stalled_counter = 0;
        already_there = false;

        if ((dir != direction) && (type != MOT_PAP_TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = MOT_PAP_TYPE_FREE_RUNNING;
        dir = direction;
        gpios.direction.set_pin_state(dir);
        requested_freq = free_run_freqs[speed] * 1000;

        tmr.stop();
        tmr.set_freq(requested_freq);
        tmr.start();
        lDebug(Info, "%s: FREE RUN, speed: %li, direction: %s", name,
                requested_freq, dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
    } else {
        lDebug(Warn, "%s: chosen speed out of bounds %li", name, speed);
    }
}

void mot_pap::move_steps(enum direction direction, uint32_t speed,
        uint32_t steps) {
    if (mot_pap::free_run_speed_ok(speed)) {
        stalled = false; // If a new command was received, assume we are not stalled
        stalled_counter = 0;
        already_there = false;

        if ((dir != direction) && (type != MOT_PAP_TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = MOT_PAP_TYPE_STEPS;
        dir = direction;
        half_steps_curr = 0;
        half_steps_requested = steps << 1;
        gpios.direction.set_pin_state(dir);
        requested_freq = free_run_freqs[speed] * 1000;
        freq_increment = requested_freq / 100;
        current_freq = freq_increment;
        half_steps_to_middle = half_steps_requested >> 1;
        max_speed_reached = false;
        ticks_last_time = xTaskGetTickCount();

        tmr.stop();
        tmr.set_freq(current_freq);
        tmr.start();
        lDebug(Info, "%s: STEPS RUN, speed: %li, direction: %s", name,
                requested_freq, dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
    } else {
        lDebug(Warn, "%s: chosen speed out of bounds %li", name, speed);
    }
}

void mot_pap::move_closed_loop(uint16_t setpoint) {
    int32_t error;
    bool already_there;
    enum direction new_dir;
    stalled = false; // If a new command was received, assume we are not stalled
    stalled_counter = 0;

    pos_cmd = setpoint;
    lDebug(Info, "%s: CLOSED_LOOP posCmd: %f posAct: %f", name, pos_cmd,
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
        if ((dir != new_dir) && (type != MOT_PAP_TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = MOT_PAP_TYPE_CLOSED_LOOP;
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
    type = MOT_PAP_TYPE_STOP;
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

            if (type == MOT_PAP_TYPE_CLOSED_LOOP) {
                int out = kp.run(pos_cmd, pos_act);
                lDebug(Info, "Control output = %i: ", out);

                enum direction dir = direction_calculate(out);
                if ((this->dir != dir) && (type != MOT_PAP_TYPE_STOP)) {
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
    if (type == MOT_PAP_TYPE_CLOSED_LOOP) {
        error = pos_cmd - pos_act;
        already_there = (abs((int) error) < 2);
    }

    if (already_there) {
        type = MOT_PAP_TYPE_STOP;
        tmr.stop();
        xSemaphoreGiveFromISR(supervisor_semaphore,
                &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        goto cont;
    }

    ++half_steps_curr;

    gpios.step.toggle();

    if ((ticks_now - ticks_last_time) > pdMS_TO_TICKS(step_time)) {

        ticks_last_time = ticks_now;
        xSemaphoreGiveFromISR(supervisor_semaphore,
                &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    cont: last_pos = pos_act;
}

/**
 * @brief 	updates the current position from RDC
 */
void mot_pap::update_position() {
    {
        if (dir == MOT_PAP_DIRECTION_CW) {
            pos_act += counts_to_inch_factor;
        } else {
            pos_act -= counts_to_inch_factor;
        }

    //  me->dir == MOT_PAP_DIRECTION_CW ? ++me->encoder_count:--me->pos_act;

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
