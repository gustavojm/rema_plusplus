#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "kp.h"
#include "limits.h"
#include "debug.h"

void kp::init(int kp,
		enum controller_direction controller_dir,
		double sample_period_ms, int min_output, int max_output, int min_abs_output)
{
	set_output_limits(min_output, max_output, min_abs_output);

	sample_period_ms = sample_period_ms;

	set_controller_direction(controller_dir);

	// Set tunings with provided constants
	set_tunings(kp);
	prev_input = 0;
	prev_output = 0;

	p_term = 0.0;
	num_times_ran = 0;
}

void kp::restart(int input) {
	prev_input = input;
	prev_error = 0;
	num_times_ran = 1;
}


int kp::run(float setpoint, float input) {
#define RAMP_RATE	0.01		//Change the setpoint by at most 0.1 per iteration

//	if (num_times_ran == 1) {
//		setpoint_jump = abs(setpoint - prev_setpoint);
//	}
//
//	if (setpoint > 0 && setpoint > (prev_setpoint + (setpoint_jump * RAMP_RATE))) {
//		setpoint = prev_setpoint + (setpoint_jump * RAMP_RATE);
//	} else if (setpoint < 0 && setpoint < (prev_setpoint - (setpoint_jump * RAMP_RATE))){
//		setpoint = prev_setpoint - (setpoint_jump * RAMP_RATE);
//	}

	int error = (setpoint - input);

	// PROPORTIONAL CALCS
	p_term = zp * error;

	output = p_term;

	// Limit output
	if (output > out_max)
		output = out_max;
	else if (output < out_min)
		output = out_min;

	// Remember input value to next call
	prev_input = input;
	// Remember last output for next call
	prev_output = output;
	prev_error = error;

	// Increment the Run() counter, after checking to make sure it hasn't reached
	// max value.
	if (num_times_ran < INT_MAX)
		num_times_ran++;

	float attenuation = (num_times_ran < 10) ? num_times_ran * 0.1 : 1;
	int out = output * attenuation;
	if (out >= 0 && out < out_abs_min) {
		return out_abs_min;
	}
	if (out < 0 && out > -out_abs_min) {
		return -out_abs_min;
	}
	return out;
}

//! @brief		Sets the KP tunings.
//! @warning	Make sure samplePeriodMs is set before calling this funciton.
void kp::set_tunings(int kp) {
	if (kp < 0)
		return;

	kp = kp;

	// Calculate time-step-scaled KP terms
	zp = kp;

	if (controller_dir == KP_REVERSE) {
		zp = (0 - zp);

	}

	lDebug(Info, "ZP: %f", zp);
}

int kp::get_kp() {
	return kp;
}


int kp::get_zp() {
	return zp;
}


void kp::set_sample_period(unsigned int new_sample_period_ms) {
	if (new_sample_period_ms > 0) {
		sample_period_ms = new_sample_period_ms;
	}
}

void kp::set_output_limits(int min, int max, int min_abs) {
	out_min = min;
	out_max = max;
	out_abs_min = min_abs;
}

void kp::set_controller_direction(enum controller_direction controller_dir) {
	if (this->controller_dir != controller_dir) {
		// Invert control constants
		zp = (0 - zp);
	}
	controller_dir = controller_dir;
}
