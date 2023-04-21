#ifndef BRESENHAM_H_
#define BRESENHAM_H_

#include <stdint.h>
#include <chrono>

#include "mot_pap.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"
#include "gpio.h"
#include "kp.h"

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

    explicit bresenham(const char *name, class tmr t) :
            name(name), tmr(t){
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
	class tmr tmr;
    int ticks_last_time = 0;
    QueueHandle_t queue;
    SemaphoreHandle_t supervisor_semaphore;
    mot_pap* leader_axis = nullptr;
    mot_pap* follower_axis = nullptr;
    class kp kp;
};

/**
 * @struct  bresenham_msg
 * @brief   messages to axis tasks.
 */
struct bresenham_msg {
    enum mot_pap::type type;
    int x_setpoint;
    int y_setpoint;
};

#endif /* BRESENHAM_H_ */
