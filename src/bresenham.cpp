#include "mot_pap.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "board.h"
#include "queue.h"
#include "task.h"

#include "bresenham.h"
#include "debug.h"
#include "rema.h"

void bresenham::task() {
    struct bresenham_msg *msg_rcv;

    while (true) {
        if (xQueueReceive(queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
            lDebug(Info, "%s: command received", name);

            switch (msg_rcv->type) {
            case mot_pap::type::MOVE:
                vTaskSuspend(supervisor_task_handle);
                was_stopped_by_probe = false;
                was_stopped_by_probe_protection = false;
                was_soft_stopped = false;
                move(msg_rcv->first_axis_setpoint, msg_rcv->second_axis_setpoint);
                vTaskResume(supervisor_task_handle);
                break;

            case mot_pap::type::SOFT_STOP:
                if (is_moving) {
                    int x2 = kp.out_max;
                    int x1 = kp.out_min;
                    int y2 = 1000;
                    int y1 = 80;
                    int x = current_freq;
                    int y = ((static_cast<float>(y2 - y1) / (x2 - x1)) * (x - x1)) + y1;

                    int counts = y;
                    lDebug(Info, "Soft stop %s in %i counts", name, counts);

                    int first_axis_setpoint = first_axis->current_counts;
                    int second_axis_setpoint = second_axis->current_counts;

                    if (first_axis->destination_counts > first_axis->current_counts) {
                        first_axis_setpoint += counts;
                    }
                    // DO NOT use "else". If destination_counts() == current_counts
                    // nothing must be done
                    if (first_axis->destination_counts < first_axis->current_counts) {
                        first_axis_setpoint -= counts;
                    }

                    if (second_axis->destination_counts > second_axis->current_counts) {
                        second_axis_setpoint += counts;
                    }
                    // DO NOT use "else". If destination_counts() == current_counts
                    // nothing must be done
                    if (second_axis->destination_counts < second_axis->current_counts) {
                        second_axis_setpoint -= counts;
                    }

                    vTaskSuspend(supervisor_task_handle);
                    was_soft_stopped = true;
                    move(first_axis_setpoint, second_axis_setpoint);
                    vTaskResume(supervisor_task_handle);

                    // first_axis->soft_stop(y);
                    // second_axis->soft_stop(y);
                }
                break;

            case mot_pap::HARD_STOP:
            default:
                stop();
                lDebug(Info, "Hard stop %s", name);
                break;
            }

            delete msg_rcv;
            msg_rcv = nullptr;
        }
    }
}

void bresenham::calculate() {
    first_axis->delta = abs(first_axis->destination_counts - first_axis->current_counts);
    second_axis->delta = abs(second_axis->destination_counts - second_axis->current_counts);

    first_axis->set_direction();
    second_axis->set_direction();

    error = first_axis->delta - second_axis->delta;

    if (first_axis->delta > second_axis->delta) {
        leader_axis = first_axis;
    } else {
        leader_axis = second_axis;
    }
}

void bresenham::move(int first_axis_setpoint, int second_axis_setpoint) {
    // Bresenham error calculation needs to multiply by 2 for the error
    // calculation, thus clamp setpoints to half INT32 min and max
    first_axis_setpoint =
        std::clamp(first_axis_setpoint, (static_cast<int>(INT32_MIN) / 2), (static_cast<int>(INT32_MAX) / 2));
    second_axis_setpoint =
        std::clamp(second_axis_setpoint, (static_cast<int>(INT32_MIN) / 2), (static_cast<int>(INT32_MAX) / 2));

    if (!rema::control_enabled_get()) {
        lDebug(Warn, "Trying to move with control disabled");
        return;
    }

    if (has_brakes) {
        if (rema::brakes_mode != rema::brakes_mode_t::ON) {
            rema::brakes_release();
        } else {
            lDebug(Warn, "Trying to move with brakes ON");
            return;
        }
    }

    is_moving = true;
    already_there = false;
    first_axis->stall_reset();
    second_axis->stall_reset();
    touching_counter = 0;
    first_axis->read_pos_from_encoder();
    second_axis->read_pos_from_encoder();
    first_axis->set_destination_counts(first_axis_setpoint);
    second_axis->set_destination_counts(second_axis_setpoint);
    lDebug(Info, "MOVE, %c: %i, %c: %i", first_axis->name, first_axis_setpoint, second_axis->name, second_axis_setpoint);

    calculate();

    if (first_axis->check_already_there() && second_axis->check_already_there()) {
        already_there = true;
        stop();
        lDebug(Info, "%s: already there", name);
    } else {
        if (!was_soft_stopped) {
            kp.restart();
            current_freq = kp.run(leader_axis->destination_counts, leader_axis->current_counts);
        } else {
            current_freq = (current_freq + kp.run_unattenuated(leader_axis->destination_counts, leader_axis->current_counts)) / 2;
        }
        lDebug(Debug, "Control output = %i: ", current_freq);

        ticks_last_time = xTaskGetTickCount();
        tmr.change_freq(current_freq);
    }
}

void bresenham::step() {
    int error2 = error << 1;
    if (error2 >= -second_axis->delta) {
        error -= second_axis->delta;
        if (!first_axis->check_already_there()) {
            first_axis->step();
        }
    }
    if (error2 <= first_axis->delta) {
        error += first_axis->delta;
        if (!second_axis->check_already_there()) {
            second_axis->step();
        }
    }
}

/**
 * @brief   supervise motor movement for stall or position reached in closed
 * loop
 * @returns nothing
 * @note    to be called by the deferred interrupt task handler
 */
void bresenham::supervise() {
    while (true) {
        if (xSemaphoreTake(supervisor_semaphore, portMAX_DELAY) == pdPASS) {

            first_axis->read_pos_from_encoder();
            second_axis->read_pos_from_encoder();

            if (rema::stall_control) {
                bool first_axis_stalled = first_axis->check_for_stall();   // make sure that both stall
                bool second_axis_stalled = second_axis->check_for_stall(); // checks are executed

                if (first_axis_stalled || second_axis_stalled) {
                    stop();
                    rema::control_enabled_set(false);
                    continue;
                }
            }

            if (rema::touch_probe_protection) {
                if (rema::is_touch_probe_touching()) {
                    touching_counter++;
                    if (touching_counter >= touching_max_count) {
                        touching_counter = 0;
                        was_stopped_by_probe_protection = true;
                        stop();
                        lDebug(Warn, "%s: touch probe protection", name);
                        continue;
                    }
                } else {
                    touching_counter = 0;
                }
            }

            // Watchdog is restarted every time telemetry is sent to REMA_Proxy
            if (rema::is_watchdog_expired()) {
                stop();
                lDebug(Info, "Watchdog expired");
                continue;
            }

            calculate(); // recalculate to compensate for encoder errors
                         // if didn't stop for proximity to set point, avoid going to
                         // infinity keeps dancing around the setpoint...

            if (!was_soft_stopped) {
                current_freq = kp.run(leader_axis->destination_counts, leader_axis->current_counts);
            } else {
                current_freq = (current_freq + kp.run_unattenuated(leader_axis->destination_counts, leader_axis->current_counts)) / 2;
            }
            lDebug(Debug, "Control output = %i: ", current_freq);
            tmr.change_freq(current_freq);
        }
    }
}

/**
 * @brief   function called by the timer ISR to generate the output pulses
 */
void bresenham::isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t ticks_now = xTaskGetTickCount();

    already_there = first_axis->check_already_there() && second_axis->check_already_there();
    if (already_there) {
        stop();
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return;
    }

    step();

    if ((ticks_now - ticks_last_time) > pdMS_TO_TICKS(step_time.count())) {
        ticks_last_time = ticks_now;
        xSemaphoreGiveFromISR(supervisor_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief   if there is a movement in process, stops it
 * @returns nothing
 */
void bresenham::stop() {
    is_moving = false;
    tmr.stop();
    current_freq = 0;
    if (has_brakes) {
        rema::brakes_apply();
    }
}

/**
 * @brief   if there is a movement in process, stops it
 * @returns nothing
 */
void bresenham::pause() {
    if (is_moving) {
        tmr.stop();
    }
}

/**
 * @brief   if there was a movement in process, resume it
 * @returns nothing
 */
void bresenham::resume() {
    if (is_moving) {
        tmr.start();
    }
}

void bresenham::send(bresenham_msg msg) {
    auto *msg_ptr = new bresenham_msg(msg);
    if (xQueueSend(queue, &msg_ptr, portMAX_DELAY) == pdPASS) {
        lDebug(Info, "%s: command sent", name);
    }
}