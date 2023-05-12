#include <stdlib.h>
#include <stdint.h>
#include <chrono>

#include "xy_axes.h"
#include "bresenham.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

#define XY_AXES_TASK_PRIORITY ( configMAX_PRIORITIES - 3 )
#define XY_AXES_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 1)


mot_pap x_axis("X");
mot_pap y_axis("Y");

tmr xy_axes_tmr = tmr(LPC_TIMER0, RGU_TIMER0_RST, CLK_MX_TIMER0, TIMER0_IRQn);
bresenham x_y_axes("xy_axes", &x_axis, &y_axis, xy_axes_tmr);

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void xy_axes_init() {
    x_axis.motor_resolution = 25000;
    x_axis.encoder_resolution = 500;
    x_axis.inches_to_counts_factor = 5000;
    x_axis.reversed = true;

    x_axis.gpios.step = gpio { 2, 1, SCU_MODE_FUNC4, 5, 1 }.init_output();      //DOUT0 P2_1    PIN81   GPIO5[1]
    x_axis.gpios.direction = gpio { 4, 5, SCU_MODE_FUNC0, 2, 6 }.init_output(); //DOUT1 P4_5    PIN10   GPIO2[6]

    y_axis.motor_resolution = 25000;
    y_axis.encoder_resolution = 500;
    y_axis.inches_to_counts_factor = 5000;

    y_axis.gpios.step = gpio {4, 6, SCU_MODE_FUNC0, 2, 5}.init_output();        //DOUT2 P4_6    PIN11   GPIO2[6]
    y_axis.gpios.direction = gpio {4, 8, SCU_MODE_FUNC4, 5, 12}.init_output();  //DOUT4 P4_8    PIN15   GPIO5[12]

    x_y_axes.kp = {100,                              //!< Kp
            kp::DIRECT,                             //!< Control type
            x_y_axes.step_time,                      //!< Update rate (ms)
            -100000,                                //!< Min output
            100000,                                 //!< Max output
            10000                                   //!< Absolute Min output
    };

    if (x_y_axes.supervisor_semaphore != NULL) {
        // Create the 'handler' task, which is the task to which interrupt processing is deferred
        xTaskCreate([](void* axes) { static_cast<bresenham*>(axes)->supervise();}, "X_Y_AXES supervisor",
        256,
        &x_y_axes, XY_AXES_SUPERVISOR_TASK_PRIORITY, NULL);
        lDebug(Info, "x_y_axes: supervisor task created");
    }

    xTaskCreate([](void* axes) { static_cast<bresenham*>(axes)->task();}, "X_Y_AXES", 256, &x_y_axes,
    XY_AXES_TASK_PRIORITY, NULL);

    lDebug(Info, "x_y_axes: task created");
}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER0_IRQHandler(void) {
    if (x_y_axes.tmr.match_pending()) {
        x_y_axes.isr();
    }
}
