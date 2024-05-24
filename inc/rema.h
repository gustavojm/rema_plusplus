#ifndef REMA_H_
#define REMA_H_

#include "FreeRTOS.h"
#include "bresenham.h"
#include "encoders_pico.h"
#include "gpio_templ.h"
#include "task.h"
#include "xy_axes.h"
#include "z_axis.h"

#define WATCHDOG_TIME_MS 1000

class rema {
public:
  enum class brakes_mode_t { OFF, AUTO, ON };

  static void init_outputs();

  static void control_enabled_set(bool status);

  static bool control_enabled_get();

  static void brakes_release();

  static void brakes_apply();

  static void touch_probe_extend();

  static void touch_probe_retract();

  static void stall_control_set(bool status);

  static bool stall_control_get();

  static void update_watchdog_timer();

  static bool is_watchdog_expired();

  static void hard_limits_reached();

  static bool control_enabled;
  static bool stall_detection;
  static brakes_mode_t brakes_mode;
  static TickType_t lastKeepAliveTicks;
};

#endif /* REMA_H_ */
