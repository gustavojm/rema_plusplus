#include "rema.h"
#include "board.h"
#include "gpio.h"

extern bresenham *x_y_axes, *z_dummy_axes;

gpio_templ<2, 1, SCU_MODE_FUNC4, 5, 1>
    brakes_out; // DOUT0 P2_1    PIN81   GPIO5[1] Bornes 4 y 5
gpio_templ<4, 6, SCU_MODE_FUNC0, 2, 6>
    touch_probe_actuator_out; // DOUT1 P4_6    PIN11   GPIO2[6] Bornes 6 y 7
gpio_templ<4, 5, SCU_MODE_FUNC0, 2, 5>
    relay_DOUT2; // DOUT2 P4_5    PIN10   GPIO2[5] Bornes 8 y 9
gpio_templ<4, 4, SCU_MODE_FUNC0, 2, 4>
    relay_DOUT3; // DOUT3 P4_4    PIN9    GPIO2[4] Bornes 10 y 11

gpio_templ<1, 5, SCU_MODE_FUNC0, 1, 8>
    shut_down_out; // DOUT7 P1_5    PIN48   GPIO1[8]

bool rema::control_enabled = false;
bool rema::stall_detection = true;
rema::brakes_mode_t rema::brakes_mode = rema::brakes_mode_t::AUTO;
TickType_t rema::lastKeepAliveTicks;

void rema::init_outputs() {
    brakes_out.init_output();
    touch_probe_actuator_out.init_output();
    relay_DOUT2.init_output();
    relay_DOUT3.init_output();
    shut_down_out.init_output();
    shut_down_out.set(1);
}

void rema::control_enabled_set(bool status) {
  control_enabled = status;  
  shut_down_out.set(!status);
}

bool rema::control_enabled_get() { return control_enabled; }

void rema::brakes_release() {
  if (brakes_mode == brakes_mode_t::AUTO || brakes_mode == brakes_mode_t::OFF) {
    brakes_out.set(1);
  }
}

void rema::brakes_apply() {
  if (brakes_mode == brakes_mode_t::AUTO || brakes_mode == brakes_mode_t::ON) {
    brakes_out.set(0);
  }
}

void rema::touch_probe_extend() {
  touch_probe_actuator_out.set(0);
}

void rema::touch_probe_retract() {
  touch_probe_actuator_out.set(1);
}

void rema::stall_control_set(bool status) { stall_detection = status; }

bool rema::stall_control_get() { return stall_detection; }

void rema::update_watchdog_timer() { lastKeepAliveTicks = xTaskGetTickCount(); }

bool rema::is_watchdog_expired() {
  return ((xTaskGetTickCount() - lastKeepAliveTicks) >
          pdMS_TO_TICKS(WATCHDOG_TIME_MS));
}

void rema::hard_limits_reached() {
  /* TODO Read input pins to determine which limit has been reached and stop
   * only one motor*/
  z_dummy_axes->stop();
  x_y_axes->stop();
}
