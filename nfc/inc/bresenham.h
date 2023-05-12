#ifndef BRESENHAM_H_
#define BRESENHAM_H_

#include <stdint.h>
#include <chrono>

#include "mot_pap.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"
#include "gpio.h"
#include "kp.h"
#include "debug.h"

#define TASK_PRIORITY ( configMAX_PRIORITIES - 3 )
#define SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 1)


using namespace std::chrono_literals;
/**
 * @struct 	mot_pap
 * @brief	axis structure.
 */
class bresenham {
public:
    enum type {
        TYPE_BRESENHAM, TYPE_FREERUN, TYPE_STOP
    };

    bresenham() = delete;

    explicit bresenham(const char *name, mot_pap* first_axis, mot_pap* second_axis, class tmr t) :
            name(name), first_axis(first_axis), second_axis(second_axis), tmr(t) {

        queue = xQueueCreate(5, sizeof(struct bresenham_msg*));
        supervisor_semaphore = xSemaphoreCreateBinary();
    }

	void task();

	void set_timer(class tmr tmr) {
		this->tmr = tmr;
	}

	void supervise();

	void move(int x_setpoint, int y_setpoint);

	void step();

	void isr();

	void stop();

	JSON_Value* json() const;

public:
	const char *name;
	enum type type;
    int requested_freq = 0;
    std::chrono::milliseconds step_time = 100ms;
    int ticks_last_time = 0;
    QueueHandle_t queue;
    SemaphoreHandle_t supervisor_semaphore;
    mot_pap* first_axis = nullptr;
    mot_pap* second_axis = nullptr;
    mot_pap* leader_axis = nullptr;
    class tmr tmr;
    volatile bool already_there = false;
    class kp kp;
    int error;

private:
    void calculate();
};

/**
 * @struct  bresenham_msg
 * @brief   messages to axis tasks.
 */
struct bresenham_msg {
    enum mot_pap::type type;
    int first_axis_setpoint;
    int second_axis_setpoint;
};

#endif /* BRESENHAM_H_ */
