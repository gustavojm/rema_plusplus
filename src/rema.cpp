#include "rema.h"
#include "board.h"
#include "gpio.h"

extern bresenham *x_y_axes, *z_dummy_axes;

gpio_templ< 2, 1, SCU_MODE_FUNC4, 5, 1 >  relay_DOUT0;          // DOUT0 P2_1    PIN81   GPIO5[1] Bornes 4 y 5
gpio_templ< 4, 6, SCU_MODE_FUNC0, 2, 6 >  relay_DOUT1;          // DOUT1 P4_6    PIN11   GPIO2[6] Bornes 6 y 7
gpio_templ< 4, 5, SCU_MODE_FUNC0, 2, 5 >  relay_DOUT2;          // DOUT2 P4_5    PIN10   GPIO2[5] Bornes 8 y 9
gpio_templ< 4, 4, SCU_MODE_FUNC0, 2, 4 >  relay_DOUT3;          // DOUT3 P4_4    PIN9    GPIO2[4] Bornes 10 y 11

gpio_templ< 1, 5, SCU_MODE_FUNC0, 1, 8 >  control_out;          // DOUT7 P1_5    PIN48   GPIO1[8]

bool rema::control_enabled = false;
bool rema::probe_enabled = false;
bool rema::stall_detection = true;
TickType_t rema::lastKeepAliveTicks;

void rema::control_enabled_set(bool status) {
    control_enabled = status;
    control_out.init_output();
    control_out.set(status);
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
