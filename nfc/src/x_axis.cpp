#include <stdlib.h>
#include <stdint.h>
#include <chrono>

#include "x_axis.h"
#include "mot_pap.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

#define X_AXIS_TASK_PRIORITY ( configMAX_PRIORITIES - 3 )
#define X_AXIS_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 1)

tmr x_axis_tmr = tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);
mot_pap x_axis("x_axis", x_axis_tmr);

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void x_axis_init() {
    x_axis.queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));
    x_axis.motor_resolution = 25000;
    x_axis.encoder_resolution = 500;
    x_axis.inches_to_counts_factor = 1000;

    x_axis.gpios.step = gpio { 2, 1, SCU_MODE_FUNC4, 5, 1 }.init_output();      //DOUT0 P2_1    PIN81   GPIO5[1]
    x_axis.gpios.direction = gpio { 4, 5, SCU_MODE_FUNC0, 2, 6 }.init_output(); //DOUT1 P4_5    PIN10   GPIO2[6]

    x_axis.kp = { 100,                              //!< Kp
            kp::DIRECT,                             //!< Control type
            x_axis.step_time,                       //!< Update rate (ms)
            -100000,                                //!< Min output
            100000,                                 //!< Max output
            10000                                   //!< Absolute Min output
            };

    x_axis.supervisor_semaphore = xSemaphoreCreateBinary();

    if (x_axis.supervisor_semaphore != NULL) {
        // Create the 'handler' task, which is the task to which interrupt processing is deferred
        xTaskCreate([](void* axis) { static_cast<mot_pap*>(axis)->supervise();}, "X_AXIS supervisor", 256,
        &x_axis, X_AXIS_SUPERVISOR_TASK_PRIORITY, NULL);
        lDebug(Info, "x_axis: supervisor task created");
    }

    xTaskCreate([](void* axis) { static_cast<mot_pap*>(axis)->task(); }, "X_AXIS", 256, &x_axis,
    X_AXIS_TASK_PRIORITY, NULL);

    lDebug(Info, "x_axis: task created");

}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER1_IRQHandler(void) {
    if (x_axis.tmr.match_pending()) {
        x_axis.isr();
    }
}
