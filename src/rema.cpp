#include "rema.h"
#include "board.h"
#include "gpio.h"

extern bresenham *x_y_axes, *z_dummy_axes;

gpio_templ< 4, 4, SCU_MODE_FUNC0, 2, 4 >  relay_4;      // DOUT3 P4_4    PIN9    GPIO2[4]>

bool rema::control_enabled = false;
bool rema::probe_enabled = false;
bool rema::stall_detection = true;
TickType_t rema::lastKeepAliveTicks;

void rema::control_enabled_set(bool status) {
    control_enabled = status;
}

bool rema::control_enabled_get() {
    return control_enabled;
}

void rema::probe_enabled_set(bool status)  {
    probe_enabled = status;
}

bool rema::probe_enabled_get()  {
    return probe_enabled;
}

void rema::stall_control_set(bool status) {
    stall_detection = status;
}

bool rema::stall_control_get() {
    return stall_detection;
}

void rema::lamp_pwr_set(bool status) {
    relay_4.init_output();
    relay_4.set(status);
}

void rema::update_watchdog_timer() {
    lastKeepAliveTicks = xTaskGetTickCount();
}

bool rema::is_watchdog_expired() {
    return ((xTaskGetTickCount() - lastKeepAliveTicks) > pdMS_TO_TICKS(WATCHDOG_TIME_MS) );
}

void rema::hard_limits_reached() {
    /* TODO Read input pins to determine which limit has been reached and stop only one motor*/
    z_dummy_axes->stop();
    x_y_axes->stop();
}
