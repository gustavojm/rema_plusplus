#ifndef REMA_H_
#define REMA_H_

#include "FreeRTOS.h"
#include "task.h"
#include "gpio_templ.h"
#include "bresenham.h"
#include "encoders_pico.h"
#include "z_axis.h"
#include "xy_axes.h"

#define WATCHDOG_TIME_MS   1000

class rema {

public:
    static void control_enabled_set(bool status);

    static bool control_enabled_get();

    static void probe_enabled_set(bool status);

    static bool probe_enabled_get();

    static void stall_control_set(bool status);

    static bool stall_control_get();

    static void update_watchdog_timer();

    static bool is_watchdog_expired();

    static void hard_limits_reached();

    static bool control_enabled;
    static bool probe_enabled;
    static bool stall_detection;
    static TickType_t lastKeepAliveTicks;
};

#endif /* REMA_H_ */


