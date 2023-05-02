#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "task.h"

#include "debug.h"
#include "rema.h"

#define MOT_PAP_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )

using namespace std::chrono_literals;

void mot_pap::task() {
    struct mot_pap_msg *msg_rcv;

    if (xQueueReceive(queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
        lDebug(Info, "%s: command received", name);

        stalled = false; // If a new command was received, assume we are not stalled
        stalled_counter = 0;
        already_there = false;
        half_pulses = 0;

        switch (msg_rcv->type) {
        case mot_pap::TYPE_FREE_RUNNING:
            move_free_run(msg_rcv->free_run_direction,
                    msg_rcv->free_run_speed);
            break;

        case mot_pap::TYPE_CLOSED_LOOP:
            move_closed_loop(
                    msg_rcv->closed_loop_setpoint
                            * inches_to_counts_factor);
            break;

        default:
            stop();
            break;
        }

        vPortFree(msg_rcv);
        msg_rcv = NULL;
    }
}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	DIRECTION_CW if error is positive
 * @returns	DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int error) {
    if (reversed) {
        return error < 0 ? mot_pap::direction::DIRECTION_CW : mot_pap::direction::DIRECTION_CCW;
    } else {
        return error < 0 ? mot_pap::direction::DIRECTION_CCW : mot_pap::direction::DIRECTION_CW;
    }
}

void mot_pap::set_direction(enum direction direction) {
    dir = direction;
    gpios.direction.set_pin_state(dir == DIRECTION_CW ? 0 : 1);
}

void mot_pap::set_direction() {
    dir = direction_calculate(pos_cmd - pos_act);
    gpios.direction.set_pin_state(dir == DIRECTION_CW ? 0 : 1);
}


/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either DIRECTION_CW or DIRECTION_CCW
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
        set_direction(direction);
        requested_freq = speed;
        tmr.change_freq(requested_freq);
        lDebug(Info, "%s: FREE RUN, speed: %i, direction: %s", name,
                requested_freq, dir == DIRECTION_CW ? "CW" : "CCW");
    } else {
        lDebug(Warn, "%s: chosen speed out of bounds %i", name, speed);
    }
}

void mot_pap::move_closed_loop(int setpoint) {
    int error;
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
        kp.restart();

        int out = kp.run(pos_cmd, pos_act);
        new_dir = direction_calculate(out);
        if ((dir != new_dir) && (type != TYPE_STOP)) {
            tmr.stop();
            vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
        }
        type = TYPE_CLOSED_LOOP;
        set_direction(new_dir);
        gpios.direction.set_pin_state(dir);
        requested_freq = abs(out);
        tmr.change_freq(requested_freq);
    }
}

void mot_pap::save_probe_pos_and_stop() {
    probe_pos = pos_act;
    probe_last_dir = last_dir;
    if (type != TYPE_STOP) {
        probe_triggered = true;
        stop();
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
}

bool mot_pap::check_for_stall() {
    const int expected_counts = ((half_pulses << 1) * encoder_resolution / motor_resolution) - MOT_PAP_STALL_THRESHOLD;
    const int pos_diff = abs((int) (pos_act - last_pos));

    if (pos_diff < expected_counts) {
        stalled_counter++;
        if (stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
            stalled_counter = 0;
            stalled = true;
            lDebug(Warn, "%s: stalled", name);
            return true;
        }
    } else {
        stalled_counter = 0;
    }

    half_pulses = 0;
    last_pos = pos_act;
    return false;
}

bool mot_pap::check_already_there() {
    int error = pos_cmd - pos_act;
    already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
    return already_there;
}

/**
 * @brief   supervise motor movement for stall or position reached in closed loop
 * @returns nothing
 * @note    to be called by the deferred interrupt task handler
 */
void mot_pap::supervise() {
    if (xSemaphoreTake(supervisor_semaphore,
            portMAX_DELAY) == pdPASS) {
        if (rema::stall_control_get()) {
            if (check_for_stall()) {
                stop();
                rema::control_enabled_set(false);
                goto end;
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
            set_direction(dir);
            requested_freq = abs(out);
            tmr.change_freq(requested_freq);
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
        already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
    }

    if (already_there) {
        stop();
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        goto cont;
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
 * @brief   updates the current position from RDC
 */
void mot_pap::update_position() {
    if (reversed) {
        (dir == DIRECTION_CW) ? pos_act-- : pos_act++;
    } else {
        (dir == DIRECTION_CW) ? pos_act++ : pos_act--;
    }
}

#ifdef SIMULATE_ENCODER
/**
 * @brief   updates the current position from the step counter
 */
void mot_pap::update_position_simulated() {
    int ratio = motor_resolution / encoder_resolution;
    if (!((half_pulses << 1) % ratio)) {
        update_position();
    }
}
#endif

void mot_pap::step() {
    ++half_pulses;

#ifdef SIMULATE_ENCODER
    update_position_simulated();
#else
    gpios.step.toggle();
#endif
}

JSON_Value* mot_pap::json() const {
    JSON_Value *ans = json_value_init_object();
    json_object_set_number(json_value_get_object(ans), "posCmd", pos_cmd);
    json_object_set_number(json_value_get_object(ans), "posAct", pos_act);
    json_object_set_boolean(json_value_get_object(ans), "stalled", stalled);
    return ans;
}
