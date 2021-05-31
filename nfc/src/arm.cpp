#include "arm.h"
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

#define ARM_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define ARM_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t arm_queue = NULL;

SemaphoreHandle_t arm_supervisor_semaphore;

mot_pap arm("arm");
static struct ad2s1210 rdc;

/**
 * @brief 	handles the arm movement.
 * @param 	par		: unused
 * @returns	never
 * @note	Receives commands from arm_queue
 */
static void arm_task(void *par)
{
	struct mot_pap_msg *msg_rcv;

	while (true) {
		if (xQueueReceive(arm_queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
			lDebug(Info, "arm: command received");

			arm.stalled = false; 		// If a new command was received, assume we are not stalled
			arm.stalled_counter = 0;
			arm.already_there = false;

			arm.read_corrected_pos();

			switch (msg_rcv->type) {
			case MOT_PAP_TYPE_FREE_RUNNING:
				arm.move_free_run(msg_rcv->free_run_direction,
						msg_rcv->free_run_speed);
				break;

			case MOT_PAP_TYPE_CLOSED_LOOP:
				arm.move_closed_loop(msg_rcv->closed_loop_setpoint);
				break;

			default:
				arm.stop();
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
static void arm_supervisor_task(void *par)
{
	while (true) {
		xSemaphoreTake(arm_supervisor_semaphore, portMAX_DELAY);
		arm.supervise();
	}
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle arm movements.
 * @returns	nothing
 */
void arm_init()
{
	arm_queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

	arm.offset = 41230;

	rdc.gpios.reset = &poncho_rdc_reset;
	rdc.gpios.sample = &poncho_rdc_sample;
	rdc.gpios.wr_fsync = &poncho_rdc_arm_wr_fsync;
	rdc.resolution = 16;
	rdc.fclkin = 8192000;
	rdc.fexcit = 2000;
	rdc.reversed = true;
	ad2s1210_init(&rdc);

	arm.rdc = &rdc;

	arm.gpios.direction = &dout_arm_dir;
	arm.gpios.pulse = &dout_arm_pulse;

	arm.tmr.started = false;
	arm.tmr.lpc_timer = LPC_TIMER1;
	arm.tmr.rgu_timer_rst = RGU_TIMER1_RST;
	arm.tmr.clk_mx_timer = CLK_MX_TIMER1;
	arm.tmr.timer_IRQn = TIMER1_IRQn;

	tmr_init(&arm.tmr);

	arm_supervisor_semaphore = xSemaphoreCreateBinary();
	arm.supervisor_semaphore = arm_supervisor_semaphore;

	if (arm_supervisor_semaphore != NULL) {
		// Create the 'handler' task, which is the task to which interrupt processing is deferred
		xTaskCreate(arm_supervisor_task, "ArmSupervisor",
		2048,
		NULL, ARM_SUPERVISOR_TASK_PRIORITY, NULL);
		lDebug(Info, "arm: supervisor task created");
	}

	xTaskCreate(arm_task, "Arm", 512, NULL,
	ARM_TASK_PRIORITY, NULL);

	lDebug(Info, "arm: task created");
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER1_IRQHandler(void)
{
	if (tmr_match_pending(&(arm.tmr))) {
		arm.isr();
	}
}

/**
 * @brief	gets arm RDC position
 * @returns	RDC position
 */
uint16_t arm_get_RDC_position()
{
	return ad2s1210_read_position(arm.rdc);
}


/**
 * @brief	sets arm offset
 * @param 	offset		: RDC position for 0 degrees
 * @returns	nothing
 */
void arm_set_offset(uint16_t offset)
{
	arm.offset = offset;
}

/**
 * @brief	returns status of the arm task.
 * @returns copy of status structure of the task
 */
struct mot_pap *arm_get_status(void)
{
	arm.read_corrected_pos();
	return &arm;
}

/**
 * @brief	returns status of the pole task in json format.
 * @returns pointer to JSON_Value
 */
JSON_Value *arm_json(void) {
	return arm.json();
}
