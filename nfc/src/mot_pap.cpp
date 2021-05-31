#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "ad2s1210.h"
#include "debug.h"
#include "relay.h"
#include "tmr.h"

// Frequencies expressed in Khz
static const uint32_t mot_pap_free_run_freqs[] = { 0, 25, 25, 25, 50, 75, 75,
		100, 125 };

mot_pap::mot_pap(const char *name)
{
	this->name = name;
	type = MOT_PAP_TYPE_STOP;
	last_dir = MOT_PAP_DIRECTION_CW;
	half_pulses = 0;
}

/**
 * @brief	corrects possible offsets of RDC alignment.
 * @param 	pos		: current RDC position
 * @param 	offset	: RDC value for 0 degrees
 * @returns	the offset corrected position
 */
uint16_t mot_pap::offset_correction(uint16_t pos, uint16_t offset,
		uint8_t resolution)
{
	int32_t corrected = pos - offset;
	if (corrected < 0)
		corrected = corrected + (int32_t) (1 << resolution);
	return (uint16_t) corrected;
}

/**
 * @brief	reads RDC position taking into account offset
 * @returns	nothing
 */
void mot_pap::read_corrected_pos()
{
	posAct = offset_correction(rdc->read_position(), offset,
			rdc->resolution);
}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	MOT_PAP_DIRECTION_CW if error is positive
 * @returns	MOT_PAP_DIRECTION_CCW if error is negative
 */
enum mot_pap::direction mot_pap::direction_calculate(int32_t error)
{
	return error < 0 ? MOT_PAP_DIRECTION_CW : MOT_PAP_DIRECTION_CCW;
}

/**
 * @brief 	checks if the required FREE RUN speed is in the allowed range
 * @param 	speed : the requested speed
 * @returns	true if speed is in the allowed range
 */
bool mot_pap::free_run_speed_ok(uint32_t speed)
{
	return ((speed > 0) && (speed <= MOT_PAP_MAX_SPEED_FREE_RUN));
}

/**
 * @brief 	supervise motor movement for stall or position reached in closed loop
 * @returns nothing
 * @note	to be called by the deferred interrupt task handler
 */
void mot_pap::supervise()
{
	int32_t error;
	bool already_there;
	enum mot_pap::direction dir;

	posAct = offset_correction(rdc->read_position(), offset,
			rdc->resolution);

	if (stall_detection) {
		if (abs((int) (posAct - last_pos)) < MOT_PAP_STALL_THRESHOLD) {

			stalled_counter++;
			if (stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
				stalled = true;
				tmr->stop();
				relay_main_pwr(0);
				lDebug(Warn, "%s: stalled", name);
				goto cont;
			}
		} else {
			stalled_counter = 0;
		}
	}

	if (type == MOT_PAP_TYPE_CLOSED_LOOP) {
		error = posCmd - posAct;

		if ((abs((int) error) < MOT_PAP_POS_PROXIMITY_THRESHOLD)) {
			freq = MOT_PAP_MAX_FREQ / 4;
			tmr->stop();
			tmr->set_freq(MOT_PAP_MAX_FREQ / 4);
			tmr->start();
		}

		already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);

		if (already_there) {
			already_there = true;
			type = MOT_PAP_TYPE_STOP;
			tmr->stop();
			lDebug(Info, "%s: position reached", name);
		} else {
			dir = direction_calculate(error);
			if (this->dir != dir) {
				tmr->stop();
				vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
				dir = dir;
				gpios.direction(dir);
				tmr->start();
			}
		}
	}
cont:
	last_pos = posAct;
}

void mot_pap::new_cmd_received()
{
	stalled = false; // If a new command was received, assume we are not stalled
	stalled_counter = 0;
	already_there = false;

	read_corrected_pos();
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
		if ((dir != direction) && (type != MOT_PAP_TYPE_STOP)) {
			tmr->stop();
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		type = MOT_PAP_TYPE_FREE_RUNNING;
		dir = direction;
		gpios.direction(dir);
		freq = mot_pap_free_run_freqs[speed] * 1000;

		tmr->stop();
		tmr->set_freq(freq);
		tmr->start();
		lDebug(Info, "%s: FREE RUN, speed: %lu, direction: %s", name, freq,
				dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %lu", name, speed);
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
	enum mot_pap::direction dir;

	posCmd = setpoint;
	lDebug(Info, "%s: CLOSED_LOOP posCmd: %u posAct: %u", name, posCmd, posAct);

	//calculate position error
	error = posCmd - posAct;
	already_there = (abs(error) < MOT_PAP_POS_THRESHOLD);

	if (already_there) {
		already_there = true;
		tmr->stop();
		lDebug(Info, "%s: already there", name);
	} else {
		dir = direction_calculate(error);
		if ((this->dir != dir) && (type != MOT_PAP_TYPE_STOP)) {
			tmr->stop();
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		type = MOT_PAP_TYPE_CLOSED_LOOP;
		dir = dir;
		gpios.direction(dir);
		freq = MOT_PAP_MAX_FREQ;
		tmr->stop();
		tmr->set_freq(freq);
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
	BaseType_t xHigherPriorityTaskWoken;

	if (dir != last_dir) {
		half_pulses = 0;
		last_dir = dir;
	}

	gpios.pulse();

	if (++(half_pulses) == MOT_PAP_SUPERVISOR_RATE) {
		half_pulses = 0;
		xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/**
 * @brief 	updates the current position from RDC
 */
void mot_pap::update_position()
{
	posAct = offset_correction(rdc->read_position(), offset,
			rdc->resolution);
}

JSON_Value* mot_pap::json()
{
	JSON_Value *ans = json_value_init_object();
	json_object_set_number(json_value_get_object(ans), "posCmd", posCmd);
	json_object_set_number(json_value_get_object(ans), "posAct", posAct);
	json_object_set_boolean(json_value_get_object(ans), "stalled", stalled);
	json_object_set_number(json_value_get_object(ans), "offset", offset);
	return ans;
}

