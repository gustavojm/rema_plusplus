#include "y_axis.h"
#include "mot_pap.h"

#include <stdlib.h>
#include <stdint.h>
#include <chrono>

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
    struct mot_pap_msg *msg_rcv;

    while (true) {
        if (xQueueReceive(y_axis.queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
            lDebug(Info, "y_axis: command received");

            y_axis.stalled = false;         // If a new command was received, assume we are not stalled
            y_axis.stalled_counter = 0;
            y_axis.already_there = false;

            using namespace std::chrono_literals;
            y_axis.step_time = 100ms;

            //mot_pap_read_corrected_pos(&y_axis);

            switch (msg_rcv->type) {
            case mot_pap::TYPE_FREE_RUNNING:
                y_axis.move_free_run(msg_rcv->free_run_direction,
                        msg_rcv->free_run_speed);
                break;

            case mot_pap::TYPE_CLOSED_LOOP:
                y_axis.move_closed_loop(msg_rcv->closed_loop_setpoint);
                break;

            case mot_pap::TYPE_STEPS:
                y_axis.move_steps(msg_rcv->free_run_direction,
                        msg_rcv->free_run_speed, msg_rcv->steps);
                break;

            default:
                y_axis.stop();
                break;
            }

            vPortFree(msg_rcv);
            msg_rcv = NULL;
        }
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
      y_axis.counts_to_inch_factor = (double) 1 / 1000000;
      y_axis.half_pulses = 0;
      y_axis.pos_act = 0;

      y_axis.gpios.direction = gpio { 4, 5, SCU_MODE_FUNC0, 2, 6 };     //DOUT1 P4_5    PIN10   GPIO2[5]
      y_axis.gpios.step = gpio { 4, 8, SCU_MODE_FUNC4, 5, 12 };         //DOUT4 P4_8   PIN15    GPIO5[12]  Y_AXIS_STEP

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
          xTaskCreate(y_axis_supervisor_task, "XAxisSupervisor",
          2048,
          NULL, Y_AXIS_SUPERVISOR_TASK_PRIORITY, NULL);
          lDebug(Info, "y_axis: supervisor task created");
      }

      xTaskCreate(y_axis_task, "y_AXIS", 512, NULL,
      Y_AXIS_TASK_PRIORITY, NULL);

      lDebug(Info, "y_axis: task created");

}


/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER2_IRQHandler(void) {
	if (y_axis.tmr.match_pending()) {
		y_axis.isr();
	}
}

