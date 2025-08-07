#include "kp.h"

kp::kp(int kp, int steps, std::chrono::milliseconds sample_period_ms, int normal_min, int normal_max, int slow_min, int slow_max) {
    set_output_limits(normal_min, normal_max, slow_min, slow_max);

    sample_period_ms = sample_period_ms;

    // Set tunings with provided constants
    set_tunings(kp, steps);
}

void kp::restart() {
    num_times_ran = 0;
}

int kp::run(int setpoint, int input, enum mot_pap::speed speed) {
    int out_min;
    int out_max;
    float ramp_rate = 1 / static_cast<float>(kp::ramp_steps);

    switch (speed) {
    case mot_pap::speed::SLOW:
        out_min = slow_out_min;
        out_max = slow_out_max;
        break;

    case mot_pap::speed::NORMAL:
    default:
        out_min = normal_out_min;
        out_max = normal_out_max;
        break;
    }

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

    float attenuation = (num_times_ran < ramp_steps) ? num_times_ran * ramp_rate : 1;
    int out = output * attenuation;
    if (out < out_min) {
        return out_min;
    }
    return out;
}

int kp::run_unattenuated(int setpoint, int input, enum mot_pap::speed speed) {
    int out_min;
    int out_max;

    switch (speed) {
    case mot_pap::speed::SLOW:
        out_min = slow_out_min;
        out_max = slow_out_max;
        break;

    case mot_pap::speed::NORMAL:
    default:
        out_min = normal_out_min;
        out_max = normal_out_max;
        break;
    }

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
void kp::set_tunings(float kp, int steps) {
    if (kp < 0 || steps < 0)
        return;

    kp_ = kp;
    ramp_steps = steps;
    
    // Printing floats generates hard faults...
    // lDebug(Info, "KP: %f", kp_);
}

void kp::set_sample_period(std::chrono::milliseconds new_sample_period_ms) {
    if (new_sample_period_ms > std::chrono::milliseconds(0)) {
        sample_period_ms = new_sample_period_ms;
    }
}

void kp::set_output_limits(int normal_min, int normal_max, int slow_min, int slow_max) {
    normal_out_min = normal_min;
    normal_out_max = normal_max;

    slow_out_min = slow_min;
    slow_out_max = slow_max;
}
