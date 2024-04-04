//#include <stdlib.h>
#include <cstdint>
#include <chrono>

#include "z_axis.h"
#include "bresenham.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

mot_pap z_axis('Z');
mot_pap dummy_axis('D', true);

tmr z_dummy_axes_tmr = tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);

bresenham& z_dummy_axes_get_instance() {
    static bresenham z_dummy_axes("z_dummy_axes", &z_axis, &dummy_axis, z_dummy_axes_tmr);
    return z_dummy_axes;
}

/**
 * @brief 	initializes the stepper motors for bresenham control
 * @returns	nothing
 */
bresenham& z_axis_init() {
    z_axis.motor_resolution = 25000;
    z_axis.encoder_resolution = 500;
    z_axis.inches_to_counts_factor = 5000;

    // As in arquitecture 
    // z_axis.gpios.step = gpio {4, 9, SCU_MODE_FUNC4, 5, 13}.init_output();        //DOUT5 P4_9    PIN33   GPIO5[13]
    // z_axis.gpios.direction = gpio {4, 10, SCU_MODE_FUNC4, 5, 14}.init_output();  //DOUT6 P4_10   PIN35   GPIO5[14]

    z_axis.gpios.step = gpio {4, 10, SCU_MODE_FUNC4, 5, 14}.init_output();          //DOUT6 P4_10   PIN35   GPIO5[14]
    z_axis.gpios.direction = gpio {4, 6, SCU_MODE_FUNC0, 2, 5}.init_output();       //DOUT2 P4_6    PIN11   GPIO2[6]

    bresenham& z_dummy_axes = z_dummy_axes_get_instance();
    z_dummy_axes.kp = {100,                         //!< Kp
            z_dummy_axes.step_time,                 //!< Update rate (ms)
            10000,                                  //!< Min output
            100000                                  //!< Max output
    };

    return z_dummy_axes;
}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER1_IRQHandler(void) {
    bresenham& z_dummy_axes = z_dummy_axes_get_instance();
    if (z_dummy_axes.tmr.match_pending()) {
        z_dummy_axes.isr();
    }
}
