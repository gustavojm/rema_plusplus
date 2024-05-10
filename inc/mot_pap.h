#ifndef MOT_PAP_H_
#define MOT_PAP_H_

#include <chrono>
#include <cstdint>
#include <cstdlib>

#include "FreeRTOS.h"
#include "encoders_pico.h"
#include "gpio.h"
#include "parson.h"
#include "semphr.h"
#include "tmr.h"

#define MOT_PAP_MAX_FREQ 500000
#define MOT_PAP_COMPUMOTOR_MAX_FREQ 500000 // 300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS 500

#define MOT_PAP_POS_THRESHOLD 10
#define MOT_PAP_STALL_THRESHOLD 3
#define MOT_PAP_STALL_MAX_COUNT 5

extern encoders_pico *encoders;

using namespace std::chrono_literals;
/**
 * @struct 	mot_pap
 * @brief	axis structure.
 */
class mot_pap {
public:
  enum direction {
    DIRECTION_CW,
    DIRECTION_CCW,
    DIRECTION_NONE,
  };

  enum type {
    TYPE_FREE_RUNNING,
    TYPE_CLOSED_LOOP,
    TYPE_HARD_STOP,
    TYPE_BRESENHAM,
    TYPE_SOFT_STOP
  };

  /**
   * @struct 	mot_pap_gpios
   * @brief	pointers to functions to handle GPIO lines of this stepper
   * motor.
   */
  struct gpios {
    gpio step;
  };

  mot_pap() = delete;

  explicit mot_pap(const char name, bool is_dummy = false)
      : name(name), is_dummy(is_dummy) {}

  enum direction direction_calculate(int error);

  void set_position(double pos) {
    int counts = static_cast<int>(
        pos * inches_to_counts_factor); // Thread safety is important here
    encoders->set_counter(name, counts);
    current_counts = counts; // Touch current_counts in just one place
  }

  void read_pos_from_encoder();

  void set_gpios(struct gpios gpios) { this->gpios = gpios; }

  void set_direction(enum direction direction);

  void set_direction();

  void set_destination_counts(int target) {
    int error = target - current_counts;
    already_there = (std::abs(error) < MOT_PAP_POS_THRESHOLD);

    destination_counts = target;
    if (is_dummy) {
      return;
    }
    encoders->set_target(name, target);
  }

  void step();

  void update_position();

  void update_position_simulated();

  bool check_for_stall();

  void stall_reset();

  bool check_already_there();

public:
  const char name;
  enum type type = TYPE_HARD_STOP;
  enum direction dir = DIRECTION_NONE;
  int last_pos = 0;
  int inches_to_counts_factor = 0;
  int motor_resolution = 0;
  int encoder_resolution = 0;
  int stalled_counter = 0;
  int delta = 0;
  struct gpios gpios;
  enum direction last_dir = DIRECTION_NONE;
  unsigned int half_pulses_stall =
      0; // counts steps from the last call to stall control
  unsigned int half_pulses = 0; // counts steps for encoder simulation
  bool already_there = false;
  bool stalled = false;
  bool reversed = false;
  volatile int current_counts = 0;
  int destination_counts = 0;
  bool is_dummy;
};

#endif /* MOT_PAP_H_ */
