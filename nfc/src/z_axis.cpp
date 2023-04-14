#include <stdlib.h>
#include <stdint.h>
#include <chrono>

#include "z_axis.h"
#include "mot_pap.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

#define Z_AXIS_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define Z_AXIS_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t z_axis_queue = NULL;

tmr z_axis_tmr = tmr(LPC_TIMER3, RGU_TIMER3_RST, CLK_MX_TIMER3, TIMER3_IRQn);
mot_pap z_axis("z_axis", z_axis_tmr);

/**
 * @brief   handles the X axis movement.
 * @param   par     : unused
 * @returns never
 * @note    Receives commands from z_axis_queue
 */
static void z_axis_task(void *par)
{
    while (true) {
            z_axis.task();
    }
}

/**
 * @brief   checks if stalled and if position reached in closed loop.
 * @param   par : unused
 * @returns never
 */
static void z_axis_supervisor_task(void *par)
{
    while (true) {
        xSemaphoreTake(z_axis.supervisor_semaphore, portMAX_DELAY);
        z_axis.supervise();
    }
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void z_axis_init() {
    z_axis.queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

      z_axis.type = mot_pap::TYPE_STOP;
      z_axis.reversed = false;
      z_axis.inches_to_counts_factor = 1000;
      z_axis.half_pulses = 0;
      z_axis.pos_act = 0;

      z_axis.gpios.step = gpio {4, 9, SCU_MODE_FUNC4, 5, 13}.init_output();            //DOUT5 P4_9    PIN33   GPIO5[13]
      z_axis.gpios.direction = gpio {4, 10, SCU_MODE_FUNC4, 5, 14}.init_output();      //DOUT6 P4_10   PIN35   GPIO5[14]

      z_axis.gpios.direction.init_output();
      z_axis.gpios.step.init_output();

      z_axis.kp = {100,                               //!< Kp
              kp::DIRECT,                             //!< Control type
              z_axis.step_time,                       //!< Update rate (ms)
              -100000,                                //!< Min output
              100000,                                 //!< Max output
              10000                                   //!< Absolute Min output
      };

      z_axis.supervisor_semaphore = xSemaphoreCreateBinary();

      if (z_axis.supervisor_semaphore != NULL) {
          // Create the 'handler' task, which is the task to which interrupt processing is deferred
          xTaskCreate(z_axis_supervisor_task, "Z_AXIS supervisor",
          256,
          NULL, Z_AXIS_SUPERVISOR_TASK_PRIORITY, NULL);
          lDebug(Info, "z_axis: supervisor task created");
      }

      xTaskCreate(z_axis_task, "Z_AXIS", 256, NULL,
      Z_AXIS_TASK_PRIORITY, NULL);

      lDebug(Info, "z_axis: task created");

}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER3_IRQHandler(void) {
    if (z_axis.tmr.match_pending()) {
        z_axis.isr();
    }
}
