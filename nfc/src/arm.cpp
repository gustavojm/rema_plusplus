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

#define ARM_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define ARM_SUPERVISOR_TASK_PRIORITY (configMAX_PRIORITIES - 3)

QueueHandle_t arm_queue = NULL;

SemaphoreHandle_t arm_supervisor_semaphore;

mot_pap arm("arm");
static ad2s1210 rdc;
tmr tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);
struct mot_pap::gpios gpios;
ad2s1210_gpios rdc_gpios;

/**
 * @brief 	handles the arm movement.
 * @param 	par		: unused
 * @returns	never
 * @note	Receives commands from arm_queue
 */
static void arm_task(void *par) {
    struct mot_pap::msg *msg_rcv;

    while (true) {
        if (xQueueReceive(arm_queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
            lDebug(Info, "arm: command received");

            arm.new_cmd_received();
            switch (msg_rcv->type) {
            case mot_pap::MOT_PAP_TYPE_FREE_RUNNING:
                arm.move_free_run(msg_rcv->free_run_direction,
                        msg_rcv->free_run_speed);
                break;

            case mot_pap::MOT_PAP_TYPE_CLOSED_LOOP:
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
static void arm_supervisor_task(void *par) {
    while (true) {
        xSemaphoreTake(arm_supervisor_semaphore, portMAX_DELAY);
        arm.supervise();
    }
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle arm movements.
 * @returns	nothing
 */
void arm_init() {
    arm_queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

    arm.set_offset(41230);

    rdc_gpios.reset = &poncho_rdc_reset;
    rdc_gpios.sample = &poncho_rdc_sample;
    rdc_gpios.wr_fsync = &poncho_rdc_arm_wr_fsync;
    rdc.set_gpios(rdc_gpios);

    rdc.init();

    arm.set_rdc(&rdc);

    gpios.direction = &dout_arm_dir;
    gpios.pulse = &dout_arm_pulse;
    arm.set_gpios(gpios);

    arm.set_timer(&tmr);

    arm_supervisor_semaphore = xSemaphoreCreateBinary();
    arm.set_supervisor_semaphore(arm_supervisor_semaphore);

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
void TIMER1_IRQHandler(void) {
    if (tmr.match_pending()) {
        arm.isr();
    }
}

/**
 * @brief	gets arm RDC position
 * @returns	RDC position
 */
uint16_t arm_get_RDC_position() {
    return rdc.read_position();
}

/**
 * @brief	sets arm offset
 * @param 	offset		: RDC position for 0 degrees
 * @returns	nothing
 */
void arm_set_offset(uint16_t offset) {
    arm.set_offset(offset);
}

/**
 * @brief	returns status of the arm task.
 * @returns copy of status structure of the task
 */
mot_pap* arm_get_status(void) {
    arm.read_corrected_pos();
    return &arm;
}

/**
 * @brief	returns status of the pole task in json format.
 * @returns pointer to JSON_Value
 */
JSON_Value* arm_json(void) {
    return arm.json();
}
