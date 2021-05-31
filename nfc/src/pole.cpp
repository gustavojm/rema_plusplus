#include "pole.h"
#include "mot_pap.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "ad2s1210.h"
#include "debug.h"
#include "dout.h"
#include "tmr.h"
#include "spi.h"

#define POLE_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define POLE_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3 )

QueueHandle_t pole_queue = NULL;

SemaphoreHandle_t pole_supervisor_semaphore;

static struct mot_pap pole;
static struct ad2s1210 rdc;

/**
 * @brief 	handles the Pole movement.
 * @param 	par		: unused
 * @returns	never
 * @note	Receives commands from pole_queue
 */
static void pole_task(void *par)
{
	struct mot_pap_msg *msg_rcv;

	while (true) {
		if (xQueueReceive(pole_queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
			lDebug(Info, "pole: command received");

			pole.stalled = false; 		// If a new command was received, assume we are not stalled
			pole.stalled_counter = 0;
			pole.already_there = false;

			mot_pap_read_corrected_pos(&pole);

			switch (msg_rcv->type) {
			case MOT_PAP_TYPE_FREE_RUNNING:
				mot_pap_move_free_run(&pole, msg_rcv->free_run_direction,
						msg_rcv->free_run_speed);
				break;

			case MOT_PAP_TYPE_CLOSED_LOOP:
				mot_pap_move_closed_loop(&pole, msg_rcv->closed_loop_setpoint);
				break;

			default:
				mot_pap_stop(&pole);
				break;
			}

			vPortFree(msg_rcv);
			msg_rcv = NULL;
		}
	}
}

/**
 * @brief	checks if stalled and if position reached in closed loop.
 * @param 	par	: unused
 * @returns	never
 */
static void pole_supervisor_task(void *par)
{
	while (true) {
		xSemaphoreTake(pole_supervisor_semaphore, portMAX_DELAY);
		mot_pap_supervise(&pole);
	}
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle pole movements.
 * @returns	nothing
 */
void pole_init()
{
	pole_queue = xQueueCreate(2, sizeof(struct mot_pap_msg*));

	pole.name = "pole";
	pole.type = MOT_PAP_TYPE_STOP;
	pole.last_dir = MOT_PAP_DIRECTION_CW;
	pole.half_pulses = 0;
	pole.offset = 41354;

	rdc.gpios.reset = &poncho_rdc_reset;
	rdc.gpios.sample = &poncho_rdc_sample;
	rdc.gpios.wr_fsync = &poncho_rdc_pole_wr_fsync;
	rdc.resolution = 16;
	rdc.fclkin = 8192000;
	rdc.fexcit = 2000;
	rdc.reversed = true;
	ad2s1210_init(&rdc);

	pole.rdc = &rdc;

	pole.gpios.direction = &dout_pole_dir;
	pole.gpios.pulse = &dout_pole_pulse;

	pole.tmr.started = false;
	pole.tmr.lpc_timer = LPC_TIMER0;
	pole.tmr.rgu_timer_rst = RGU_TIMER0_RST;
	pole.tmr.clk_mx_timer = CLK_MX_TIMER0;
	pole.tmr.timer_IRQn = TIMER0_IRQn;

	tmr_init(&pole.tmr);

	pole_supervisor_semaphore = xSemaphoreCreateBinary();
	pole.supervisor_semaphore = pole_supervisor_semaphore;

	if (pole_supervisor_semaphore != NULL) {
		// Create the 'handler' task, which is the task to which interrupt processing is deferred
		xTaskCreate(pole_supervisor_task, "PoleSupervisor",
		2048,
		NULL, POLE_SUPERVISOR_TASK_PRIORITY, NULL);
		lDebug(Info, "pole: supervisor task created");
	}

	xTaskCreate(pole_task, "Pole", 512, NULL,
	POLE_TASK_PRIORITY, NULL);

	lDebug(Info, "pole: task created");
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER0_IRQHandler(void)
{
	if (tmr_match_pending(&(pole.tmr))) {
		mot_pap_isr(&pole);
	}
}

/**
 * @brief	gets pole RDC position
 * @returns	RDC position
 */
uint16_t pole_get_RDC_position()
{
	return ad2s1210_read_position(pole.rdc);
}


/**
 * @brief	sets pole offset
 * @param 	offset		: RDC position for 0 degrees
 * @returns	nothing
 */
void pole_set_offset(uint16_t offset)
{
	pole.offset = offset;
}

/**
 * @brief	returns status of the pole task.
 * @return 	copy of status structure of the task
 */
struct mot_pap *pole_get_status(void)
{
	mot_pap_read_corrected_pos(&pole);
	return &pole;
}

/**
 * @brief	returns status of the pole task in json format.
 * @returns pointer to JSON_Value
 */
JSON_Value *pole_json(void) {
	return mot_pap_json(&pole);
}
