#include "x_axis.h"
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

#define X_AXIS_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define X_AXIS_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t x_axis_queue = NULL;

tmr x_axis_tmr = tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);
mot_pap x_axis("x_axis", x_axis_tmr);

/**
 * @brief   handles the X axis movement.
 * @param   par     : unused
 * @returns never
 * @note    Receives commands from x_axis_queue
 */
static void x_axis_task(void *par)
{
    struct mot_pap_msg *msg_rcv;

    while (true) {
        if (xQueueReceive(x_axis.queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
            lDebug(Info, "x_axis: command received");

            x_axis.stalled = false;         // If a new command was received, assume we are not stalled
            x_axis.stalled_counter = 0;
            x_axis.already_there = false;

            using namespace std::chrono_literals;
            x_axis.step_time = 100ms;

            //mot_pap_read_corrected_pos(&x_axis);

            switch (msg_rcv->type) {
            case mot_pap::TYPE_FREE_RUNNING:
                x_axis.move_free_run(msg_rcv->free_run_direction,
                        msg_rcv->free_run_speed);
                break;

            case mot_pap::TYPE_CLOSED_LOOP:
                x_axis.move_closed_loop(msg_rcv->closed_loop_setpoint);
                break;

            case mot_pap::TYPE_STEPS:
                x_axis.move_steps(msg_rcv->free_run_direction,
                        msg_rcv->free_run_speed, msg_rcv->steps);
                break;

            default:
                x_axis.stop();
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
static void x_axis_supervisor_task(void *par)
{
    while (true) {
        xSemaphoreTake(x_axis.supervisor_semaphore, portMAX_DELAY);
        x_axis.supervise();
    }
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void x_axis_init() {
    x_axis.queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

      x_axis.type = mot_pap::TYPE_STOP;
      x_axis.counts_to_inch_factor = (double) 1 / 1000000;
      x_axis.half_pulses = 0;
      x_axis.pos_act = 0;

      x_axis.gpios.direction = gpio { 4, 5, SCU_MODE_FUNC0, 2, 6 };     //DOUT1 P4_5    PIN10   GPIO2[5]
      x_axis.gpios.step = gpio { 4, 8, SCU_MODE_FUNC4, 5, 12 };         //DOUT4 P4_8   PIN15   GPIO5[12]  X_AXIS_STEP

      x_axis.gpios.direction.init_output();
      x_axis.gpios.step.init_output();

      x_axis.kp = {100,                               //!< Kp
              kp::DIRECT,                             //!< Control type
              x_axis.step_time,                       //!< Update rate (ms)
              -100000,                                //!< Min output
              100000,                                 //!< Max output
              10000                                   //!< Absolute Min output
      };

      x_axis.supervisor_semaphore = xSemaphoreCreateBinary();

      if (x_axis.supervisor_semaphore != NULL) {
          // Create the 'handler' task, which is the task to which interrupt processing is deferred
          xTaskCreate(x_axis_supervisor_task, "XAxisSupervisor",
          2048,
          NULL, X_AXIS_SUPERVISOR_TASK_PRIORITY, NULL);
          lDebug(Info, "x_axis: supervisor task created");
      }

      xTaskCreate(x_axis_task, "X_AXIS", 512, NULL,
      X_AXIS_TASK_PRIORITY, NULL);

      lDebug(Info, "x_axis: task created");

}


/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER1_IRQHandler(void) {
	if (x_axis.tmr.match_pending()) {
		x_axis.isr();
	}
}

