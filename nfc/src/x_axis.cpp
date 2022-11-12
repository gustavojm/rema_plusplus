#include "x_axis.h"
#include "mot_pap.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

#define X_AXIS_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t x_axis_queue = NULL;

mot_pap x_axis("x_axis");

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void x_axis_init()
{
	x_axis.type = mot_pap::type::MOT_PAP_TYPE_STOP;
	x_axis.last_dir = mot_pap::direction::MOT_PAP_DIRECTION_CW;
	x_axis.half_pulses = 0;
	x_axis.offset = 41230;

	x_axis.gpios.direction = (struct gpio_entry) { 4, 8, SCU_MODE_FUNC4, 5, 12 };		//DOUT4 P4_8 	PIN15  	GPIO5[12]   X_AXIS_DIR
	x_axis.gpios.step = (struct gpio_entry) { 4, 9, SCU_MODE_FUNC4, 5, 13 };			//DOUT5 P4_9  	PIN33	GPIO5[13] 	X_AXIS_STEP

	gpio_init_output(x_axis.gpios.direction);
	gpio_init_output(x_axis.gpios.step);

	tmr x_axis_tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);

	x_axis.tmr = &x_axis_tmr;
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER1_IRQHandler(void)
{
	if (x_axis.tmr->match_pending()) {
		x_axis.isr();
	}
}

