#ifndef REMA_H_
#define REMA_H_

#include "FreeRTOS.h"
#include "task.h"
#include "gpio_templ.h"
#include "bresenham.h"
#include "z_axis.h"
#include "xy_axes.h"

#define WATCHDOG_TIME_MS   1000

class rema {

    using palper_t = gpio_pinint_templ<4, 0, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 0, PIN_INT3_IRQn>;

public:
    static void control_enabled_set(bool status);

    static bool control_enabled_get();

    static void probe_enabled_set(bool status);

    static bool probe_enabled_get();

    static void stall_control_set(bool status);

    static bool stall_control_get();

    void lamp_pwr_set(bool status);

    static void update_watchdog_timer();

    static bool is_watchdog_expired();

    static void hard_limits_init(void);

    static bool control_enabled;
    static bool probe_enabled;
    static bool stall_detection;
    static TickType_t lastKeepAliveTicks;

    static struct hard_limits {
        gpio_templ<4, 2, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 2> z_out;      //DIN2 P4_2     PIN8     GPIO2[2]    ZSz-
        gpio_templ<4, 3, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 3> z_in;       //DIN3 P4_3     PIN7     GPIO2[3]    ZSz+
        gpio_templ<7, 3, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 11> y_up;      //DIN4 P7_3     PIN117   GPIO3[11]   ZSz-
        gpio_templ<7, 4, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 12> y_down;    //DIN5 P7_4     PIN132   GPIO3[12]   ZSy+
        gpio_templ<7, 5, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 13> x_right;   //DIN6 P7_5     PIN133   GPIO3[13]   ZSy-
        gpio_templ<7, 6, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 14> x_left;    //DIN7 P7_6     PIN134   GPIO3[14]   ZSx+
    } hard_limits;

    static palper_t palper;
};

#endif /* REMA_H_ */


