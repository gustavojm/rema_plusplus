#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"
#include "relay.h"
#include "tmr.h"

// Frequencies expressed in Khz
const uint32_t mot_pap::free_run_freqs[] =
		{ 0, 25, 25, 25, 50, 75, 75, 100, 125 };

#define MOT_PAP_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define MOT_PAP_HELPER_TASK_PRIORITY ( configMAX_PRIORITIES - 3)
QueueHandle_t isr_helper_task_queue = NULL;

extern int count_a;

/**
 * @brief 	supervise motor movement for stall or position reached in closed loop
 * @param 	me			: struct mot_pap pointer
 * @returns nothing
 * @note	to be called by the deferred interrupt task handler
 */
void isr_helper_task(void *par)
{
	mot_pap *me;
	while (true) {
		if (xQueueReceive(isr_helper_task_queue, &me,
		portMAX_DELAY) == pdPASS) {
			if (me->stalled)
				lDebug(Warn, "%s: stalled", me->name);

			if (me->already_there)
				lDebug(Info, "%s: position reached", me->name);

		}
	}
}

mot_pap::mot_pap(const char *name) :
		name(name), type(MOT_PAP_TYPE_STOP), last_dir(MOT_PAP_DIRECTION_CW), half_pulses(
				0), offset(0)
{
}

void mot_pap::init()
{
	isr_helper_task_queue = xQueueCreate(1, sizeof(struct mot_pap*));
	if (isr_helper_task_queue != NULL) {
		// Create the 'handler' task, which is the task to which interrupt processing is deferred
		xTaskCreate(isr_helper_task, "MotPaPHelper", 2048,
		NULL, MOT_PAP_HELPER_TASK_PRIORITY, NULL);
		lDebug(Info, "mot_pap: helper task created");
	}
}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	MOT_PAP_DIRECTION_CW if error is positive
 * @returns	MOT_PAP_DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int32_t error) const
{
	return error < 0 ? MOT_PAP_DIRECTION_CW : MOT_PAP_DIRECTION_CCW;
}

/**
 * @brief 	checks if the required FREE RUN speed is in the allowed range
 * @param 	speed : the requested speed
 * @returns	true if speed is in the allowed range
 */
bool mot_pap::free_run_speed_ok(uint32_t speed) const
{
	return ((speed > 0) && (speed <= MOT_PAP_MAX_SPEED_FREE_RUN));
}

/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either MOT_PAP_DIRECTION_CW or MOT_PAP_DIRECTION_CCW
 * @param 	speed		: integer from 0 to 8
 * @returns	nothing
 */
void mot_pap::move_free_run(enum direction direction, uint32_t speed)
{
	if (free_run_speed_ok(speed)) {
		stalled = false; // If a new command was received, assume we are not stalled
		stalled_counter = 0;
		already_there = false;

		if ((dir != direction) && (type != MOT_PAP_TYPE_STOP)) {
			tmr->stop();
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		type = MOT_PAP_TYPE_FREE_RUNNING;
		dir = direction;
		gpio_set_pin_state(gpios.direction, dir);
		requested_freq = free_run_freqs[speed] * 1000;

		tmr->stop();
		tmr->set_freq(requested_freq);
		tmr->start();
		lDebug(Info, "%s: FREE RUN, speed: %li, direction: %s", name,
				requested_freq, dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %li", name, speed);
	}
}

void mot_pap::move_steps(enum direction direction, uint32_t speed,
		uint32_t steps)
{
	if (mot_pap::free_run_speed_ok(speed)) {
		stalled = false; // If a new command was received, assume we are not stalled
		stalled_counter = 0;
		already_there = false;

//		if ((dir != direction) && (type != MOT_PAP_TYPE_STOP)) {
//			tmr->stop();
//			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
//		}
		type = MOT_PAP_TYPE_STEPS;
		dir = direction;
		half_steps_curr = 0;
		half_steps_requested = steps << 1;
		gpio_set_pin_state(gpios.direction, dir);
		requested_freq = free_run_freqs[speed] * 1000;
		freq_increment = requested_freq / 100;
		current_freq = freq_increment;
		half_steps_to_middle = half_steps_requested >> 1;
		max_speed_reached = false;
		ticks_last_time = xTaskGetTickCount();

		tmr->stop();
		tmr->set_freq(current_freq);
		tmr->start();
		lDebug(Info, "%s: STEPS RUN, speed: %li, direction: %s", name,
				requested_freq, dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %li", name, speed);
	}
}

/**
 * @brief	if allowed, starts a closed loop movement
 * @param 	me			: struct mot_pap pointer
 * @param 	setpoint	: the resolver value to reach
 * @returns	nothing
 */
void mot_pap::move_closed_loop(uint16_t setpoint)
{
	int32_t error;
	bool already_there;
	enum direction new_dir;
	stalled = false; // If a new command was received, assume we are not stalled
	stalled_counter = 0;

	posCmd = setpoint;
	lDebug(Info, "%s: CLOSED_LOOP posCmd: %li posAct: %li", name, posCmd,
			posAct);

	//calculate position error
	error = posCmd - posAct;
	already_there = (abs(error) < MOT_PAP_POS_THRESHOLD);

	if (already_there) {
		already_there = true;
		tmr->stop();
		lDebug(Info, "%s: already there", name);
	} else {
		new_dir = direction_calculate(error);
		if ((dir != new_dir) && (type != MOT_PAP_TYPE_STOP)) {
			tmr->stop();
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		type = MOT_PAP_TYPE_CLOSED_LOOP;
		dir = new_dir;
		gpio_set_pin_state(gpios.direction, dir);
		requested_freq = MOT_PAP_MAX_FREQ;
		tmr->stop();
		tmr->set_freq(requested_freq);
		tmr->start();
	}
}

/**
 * @brief	if there is a movement in process, stops it
 * @param 	me	: struct mot_pap pointer
 * @returns	nothing
 */
void mot_pap::stop()
{
	type = MOT_PAP_TYPE_STOP;
	tmr->stop();
	lDebug(Info, "%s: STOP", name);
}

/**
 * @brief 	function called by the timer ISR to generate the output pulses
 */
void mot_pap::isr()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	TickType_t ticks_now = xTaskGetTickCountFromISR();

	posAct = count_a;

	if (stall_detection) {
		if (abs((int) (posAct - last_pos)) < MOT_PAP_STALL_THRESHOLD) {

			stalled_counter++;
			if (stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
				stalled = true;
				tmr->stop();
				relay_main_pwr(0);

				xQueueSendFromISR(isr_helper_task_queue, this,
						&xHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				goto cont;
			}
		} else {
			stalled_counter = 0;
		}
	}

	if (type == MOT_PAP_TYPE_STEPS) {
		already_there = (half_steps_curr >= half_steps_requested);
	}

	int error;
	if (type == MOT_PAP_TYPE_CLOSED_LOOP) {
		error = posCmd - posAct;
		already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
	}

	if (already_there) {
		type = MOT_PAP_TYPE_STOP;
		tmr->stop();
		xQueueSendFromISR(isr_helper_task_queue, this,
				&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		return;
	}

	if ((ticks_now - ticks_last_time) > pdMS_TO_TICKS(50)) {

		bool first_half_passed = false;
		if (type == MOT_PAP_TYPE_STEPS)
			first_half_passed = half_steps_curr > (half_steps_to_middle);

		if (type == MOT_PAP_TYPE_CLOSED_LOOP)
			first_half_passed =
					(dir == MOT_PAP_DIRECTION_CW) ?
							posAct > posCmdMiddle : posAct < posCmdMiddle;

		if (!max_speed_reached && (!first_half_passed)) {
			current_freq += (freq_increment);
			if (current_freq >= requested_freq) {
				current_freq = requested_freq;
				max_speed_reached = true;
				if (type == MOT_PAP_TYPE_STEPS)
					max_speed_reached_distance = half_steps_curr;

				if (type == MOT_PAP_TYPE_CLOSED_LOOP)
					max_speed_reached_distance = posAct;

			}
		}

		int distance_left;
		if (type == MOT_PAP_TYPE_STEPS)
			distance_left = half_steps_requested - half_steps_curr;

		if (type == MOT_PAP_TYPE_CLOSED_LOOP)
			distance_left = posCmd - posAct;

		if ((max_speed_reached && distance_left <= max_speed_reached_distance)
				|| (!max_speed_reached && first_half_passed)) {
			current_freq -= (freq_increment);
			if (current_freq <= freq_increment) {
				current_freq = freq_increment;
			}
		}

		tmr->stop();
		tmr->set_freq(current_freq);
		tmr->start();
		ticks_last_time = ticks_now;
	}

	++half_steps_curr;

	gpio_toggle(gpios.step);

	cont: last_pos = posAct;
}

/**
 * @brief 	updates the current position from RDC
 */
void mot_pap::update_position()
{
	//	posAct = count_a;
}

JSON_Value* mot_pap::json() const
{
	JSON_Value *ans = json_value_init_object();
	json_object_set_number(json_value_get_object(ans), "posCmd", posCmd);
	json_object_set_number(json_value_get_object(ans), "posAct", posAct);
	json_object_set_boolean(json_value_get_object(ans), "stalled", stalled);
	json_object_set_number(json_value_get_object(ans), "offset", offset);
	return ans;
}
