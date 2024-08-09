#include <chrono>
#include <cstdint>
#include <cstdlib>

#include "board.h"
#include "debug.h"
#include "kp.h"
#include "limits.h"

const int RAMP_STEPS = 25;
const float RAMP_RATE = 1 / static_cast<float>(RAMP_STEPS);

kp::kp(int kp, std::chrono::milliseconds sample_period_ms, int min_output, int max_output) {
    set_output_limits(min_output, max_output);

    sample_period_ms = sample_period_ms;

    // Set tunings with provided constants
    set_tunings(kp);
}

void kp::restart() {
    num_times_ran = 0;
}

int kp::run(int setpoint, int input) {

    int error = std::abs(setpoint - input);

    // PROPORTIONAL CALCS
    p_term = kp_ * error;

    output = p_term;

    // Limit output
    if (output > out_max)
        output = out_max;
    else if (output < out_min)
        output = out_min;

    // Increment the Run() counter, after checking to make sure it hasn't reached
    // max value.
    if (num_times_ran < INT_MAX)
        num_times_ran++;

    float attenuation = (num_times_ran < RAMP_STEPS) ? num_times_ran * RAMP_RATE : 1;
    int out = output * attenuation;
    if (out < out_min) {
        return out_min;
    }
    return out;
}

int kp::run_unattenuated(int setpoint, int input) {

    int error = std::abs(setpoint - input);

    // PROPORTIONAL CALCS
    p_term = kp_ * error;

    output = p_term;

    // Limit output
    if (output > out_max)
        output = out_max;
    else if (output < out_min)
        output = out_min;

    return output;
}


//! @brief		Sets the KP tunings.
//! @warning	Make sure samplePeriodMs is set before calling this function.
void kp::set_tunings(float kp) {
    if (kp < 0)
        return;

    kp_ = kp;

    lDebug(Info, "KP: %f", kp_);
}

void kp::set_sample_period(std::chrono::milliseconds new_sample_period_ms) {
    if (new_sample_period_ms > std::chrono::milliseconds(0)) {
        sample_period_ms = new_sample_period_ms;
    }
}

void kp::set_output_limits(int min, int max) {
    out_min = min;
    out_max = max;
}
