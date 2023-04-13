#include <stdlib.h>
#include <stdint.h>
#include <chrono>

#include "y_axis.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

#define Y_AXIS_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define Y_AXIS_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t y_axis_queue = NULL;

tmr y_axis_tmr = tmr(LPC_TIMER2, RGU_TIMER2_RST, CLK_MX_TIMER2, TIMER2_IRQn);
mot_pap y_axis("y_axis", y_axis_tmr);

/**
 * @brief   handles the X axis movement.
 * @param   par     : unused
 * @returns never
 * @note    Receives commands from y_axis_queue
 */
static void y_axis_task(void *par)
{
    while (true) {
            y_axis.task();
    }
}

/**
 * @brief   checks if stalled and if position reached in closed loop.
 * @param   par : unused
 * @returns never
 */
static void y_axis_supervisor_task(void *par)
{
    while (true) {
        xSemaphoreTake(y_axis.supervisor_semaphore, portMAX_DELAY);
        y_axis.supervise();
    }
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void y_axis_init() {
    y_axis.queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

      y_axis.type = mot_pap::TYPE_STOP;
      y_axis.reversed = false;
      y_axis.inches_to_counts_factor = 1000;
      y_axis.half_pulses = 0;
      y_axis.pos_act = 0;

      y_axis.gpios.direction = { 4, 5, SCU_MODE_FUNC0, 2, 5 };	            //DOUT1 P4_5 	PIN10  	GPIO2[5]   Y_AXIS_DIR
      y_axis.gpios.step =  { 4, 9, SCU_MODE_FUNC4, 5, 13 };   				//DOUT5 P4_9 	PIN33  	GPIO5[13]  Y_AXIS_STEP

      y_axis.gpios.direction.init_output();
      y_axis.gpios.step.init_output();

      y_axis.kp = {100,                               //!< Kp
              kp::DIRECT,                             //!< Control type
              y_axis.step_time,                       //!< Update rate (ms)
              -100000,                                //!< Min output
              100000,                                 //!< Max output
              10000                                   //!< Absolute Min output
      };

      y_axis.supervisor_semaphore = xSemaphoreCreateBinary();

      if (y_axis.supervisor_semaphore != NULL) {
          // Create the 'handler' task, which is the task to which interrupt processing is deferred
          xTaskCreate(y_axis_supervisor_task, "Y_AXIS supervisor",
          256,
          NULL, Y_AXIS_SUPERVISOR_TASK_PRIORITY, NULL);
          lDebug(Info, "y_axis: supervisor task created");
      }

      xTaskCreate(y_axis_task, "Y_AXIS", 256, NULL,
      Y_AXIS_TASK_PRIORITY, NULL);

      lDebug(Info, "y_axis: task created");

}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
void TIMER2_IRQHandler(void) {
    if (y_axis.tmr.match_pending()) {
        y_axis.isr();
    }
}
