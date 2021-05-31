#include "lift.h"

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"
#include "debug.h"
#include "relay.h"

#define LIFT_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )

QueueHandle_t lift_queue = NULL;

static struct lift lift;

/**
 * @brief 	handles the Lift movement.
 * @param 	par		: unused
 * @returns	never
 * @note	Receives commands from lift_queue
 */
static void lift_task(void *par)
{
	struct lift_msg *msg_rcv;

	while (true) {

		if (xQueueReceive(lift_queue, &msg_rcv, portMAX_DELAY) == pdPASS) {

			switch (msg_rcv->type) {
			case LIFT_TYPE_UP:
				// HMI sends a STOP in between, so direct reverse never happens
				if (lift.type == LIFT_TYPE_DOWN) {
					relay_lift_pwr(false);
					vTaskDelay(
							pdMS_TO_TICKS(LIFT_DIRECTION_CHANGE_DELAY_MS));
				}
				// leaving this check just in case

				lift.type = LIFT_TYPE_UP;
				relay_lift_dir(LIFT_DIRECTION_UP);
				relay_lift_pwr(true);
				lDebug(Info, "lift: command received: UP");
				break;

			case LIFT_TYPE_DOWN:
				// HMI sends a STOP in between, so direct reverse never happens
				if (lift.type == LIFT_TYPE_UP) {
					relay_lift_pwr(false);
					vTaskDelay(
							pdMS_TO_TICKS(LIFT_DIRECTION_CHANGE_DELAY_MS));
				}
				// leaving this check just in case

				lift.type = LIFT_TYPE_DOWN;
				relay_lift_dir(LIFT_DIRECTION_DOWN);
				relay_lift_pwr(true);
				lDebug(Info, "lift: command received: DOWN");
				break;

			default:
				lift.type = LIFT_TYPE_STOP;
				relay_lift_pwr(false);
				lDebug(Info, "lift: command received: STOP");
				break;
			}

			vPortFree(msg_rcv);
			msg_rcv = NULL;
		}
	}
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle lift movements.
 * @returns	nothing
 */
void lift_init()
{
	relay_init();
	lift_queue = xQueueCreate(5, sizeof(struct lift_msg*));

	lift.type = LIFT_TYPE_STOP;
	lift.upLimit = false;
	lift.downLimit = false;

	xTaskCreate(lift_task, "Lift", configMINIMAL_STACK_SIZE * 2, NULL,
	LIFT_TASK_PRIORITY, NULL);
	lDebug(Info, "lift: task created");
}

/**
 * @brief	returns status of the lift task.
 * @returns	copy of status structure of the task
 */
struct lift *lift_status_get(void) /* GPa 201207 retorna (*) */
{
	return &lift; /* GPa 201207 retorna (&) */
}
