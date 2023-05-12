#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "task.h"

#include "debug.h"
#include "rema.h"

using namespace std::chrono_literals;

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

bool mot_pap::check_for_stall() {
    if (is_dummy) {
        return false;
    }

    const int expected_counts = ((half_pulses_stall >> 1) * encoder_resolution / motor_resolution) - MOT_PAP_STALL_THRESHOLD;
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

    half_pulses_stall = 0;
    last_pos = pos_act;
    return false;
}

bool mot_pap::check_already_there() {
    if (is_dummy) {
        return true;
    }

    int error = pos_cmd - pos_act;
    already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
    return already_there;
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
    if (!((half_pulses) % (ratio << 1))) {
        update_position();
    }
}
#endif

void mot_pap::step() {
    if (is_dummy) {
        return;
    }

    ++half_pulses;
    ++half_pulses_stall;

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
