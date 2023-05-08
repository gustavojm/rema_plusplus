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

tmr xy_axes_tmr = tmr(LPC_TIMER0, RGU_TIMER0_RST, CLK_MX_TIMER0, TIMER0_IRQn);
bresenham xy_axes("xy_axes", xy_axes_tmr);

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void xy_axes_init() {

    xy_axes.kp = {100,                              //!< Kp
            kp::DIRECT,                             //!< Control type
            xy_axes.step_time,                      //!< Update rate (ms)
            -100000,                                //!< Min output
            100000,                                 //!< Max output
            10000                                   //!< Absolute Min output
    };

    xy_axes.queue = xQueueCreate(5, sizeof(struct bresenham_msg*));
    xy_axes.supervisor_semaphore = xSemaphoreCreateBinary();

      if (xy_axes.supervisor_semaphore != NULL) {
          // Create the 'handler' task, which is the task to which interrupt processing is deferred
          xTaskCreate([](void * axis) { static_cast<bresenham*>(axis)->supervise();}, "XY_AXIS supervisor",
          256,
          &xy_axes, XY_AXES_SUPERVISOR_TASK_PRIORITY, NULL);
          lDebug(Info, "xy_axis: supervisor task created");
      }

      xTaskCreate([](void * axis) { static_cast<bresenham*>(axis)->task();}, "XY_AXIS", 256, &xy_axes,
      XY_AXES_TASK_PRIORITY, NULL);

      lDebug(Info, "xy_axis: task created");

}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER0_IRQHandler(void) {
    if (xy_axes.tmr.match_pending()) {
        xy_axes.isr();
    }
}
