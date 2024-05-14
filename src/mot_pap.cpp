#include "mot_pap.h"

#include <cstdint>
#include <cstdlib>

#include "board.h"
#include "task.h"

#include "debug.h"
#include "encoders_pico.h"
#include "rema.h"

extern encoders_pico *encoders;

/**
 * @brief	returns the direction of movement depending if the error is
 * positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	DIRECTION_CW if error is positive
 * @returns	DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int error) {
  if (reversed) {
    return error < 0 ? mot_pap::direction::DIRECTION_CW
                     : mot_pap::direction::DIRECTION_CCW;
  } else {
    return error < 0 ? mot_pap::direction::DIRECTION_CCW
                     : mot_pap::direction::DIRECTION_CW;
  }
}

void mot_pap::set_direction(enum direction direction) {
  dir = direction;
  if (is_dummy) {
    return;
  }
  // gpios.direction.set(dir == DIRECTION_CW ? 0 : 1);
  encoders->set_direction(name, dir == DIRECTION_CW ? 0 : 1);
}

void mot_pap::set_direction() {
  dir = direction_calculate(destination_counts - current_counts);
  set_direction(dir);
}

bool mot_pap::check_for_stall() {
  if (is_dummy) {
    return false;
  }

  const int expected_counts =
      ((half_pulses_stall >> 1) * encoder_resolution / motor_resolution) -
      MOT_PAP_STALL_THRESHOLD;
  const int pos_diff = std::abs((int)(current_counts - last_pos));

  if (pos_diff < expected_counts) {
    stalled_counter++;
    if (stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
      stalled_counter = 0;
      stalled = true;
      lDebug(Warn, "%c: stalled", name);
      return true;
    }
  } else {
    stalled_counter = 0;
  }

  half_pulses_stall = 0;
  last_pos = current_counts;
  return false;
}

void mot_pap::stall_reset() {
  stalled = false;
  stalled_counter = 0;
}

void mot_pap::read_pos_from_encoder() {
  if (is_dummy) {
    return;
  }

  current_counts = encoders->read_counter(name);
}

bool mot_pap::check_already_there() {
  if (is_dummy) {
    return true;
  }

  // int error = destination_counts() - current_counts();
  // already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
  // already_there set by encoders_pico
  return already_there;
}

/**
 * @brief   updates the current position from RDC
 */
void mot_pap::update_position() {
  if (reversed) {
    (dir == DIRECTION_CW) ? current_counts-- : current_counts++;
  } else {
    (dir == DIRECTION_CW) ? current_counts++ : current_counts--;
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
