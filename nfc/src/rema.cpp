#include "rema.h"
#include "relay.h"

void rema::control_enabled_set(bool status) {
    control_enabled = status;
    relay_main_pwr(status);
}

bool rema::control_enabled_get() {
    return control_enabled;
}

void rema::stall_control_set(bool status) {
    stall_detection = status;
}

bool rema::stall_control_get() {
    return stall_detection;
}

bool rema::control_enabled = false;
bool rema::stall_detection = true;

