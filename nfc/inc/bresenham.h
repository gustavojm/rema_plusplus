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

    bresenham() = delete;

    explicit bresenham(const char *name, mot_pap* first_axis, mot_pap* second_axis, class tmr t) :
            name(name), first_axis(first_axis), second_axis(second_axis), tmr(t) {

        queue = xQueueCreate(5, sizeof(struct bresenham_msg*));
        supervisor_semaphore = xSemaphoreCreateBinary();

        char supervisor_task_name[configMAX_TASK_NAME_LEN];
        memset(supervisor_task_name, 0, sizeof(supervisor_task_name));
        strncat(supervisor_task_name, name, sizeof(supervisor_task_name) - strlen(supervisor_task_name) - 1);
        strncat(supervisor_task_name, "_supervisor", sizeof(supervisor_task_name) - strlen(supervisor_task_name) -1);
        if (supervisor_semaphore != NULL) {
            // Create the 'handler' task, which is the task to which interrupt processing is deferred
            xTaskCreate([](void* axes) { static_cast<bresenham*>(axes)->supervise();}, supervisor_task_name,
            256,
            this, SUPERVISOR_TASK_PRIORITY, NULL);
            lDebug(Info, "%s: created", supervisor_task_name);
        }

        char task_name[configMAX_TASK_NAME_LEN];
        memset(task_name, 0, sizeof(task_name));
        strncat(task_name, name, sizeof(task_name) - strlen(task_name) - 1);
        strncat(task_name, "_task", sizeof(task_name) - strlen(task_name) - 1);
        xTaskCreate([](void* axes) { static_cast<bresenham*>(axes)->task();}, task_name, 256, this,
        TASK_PRIORITY, NULL);

        lDebug(Info, "%s: created", task_name);

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
	bool is_moving = false;
    int current_freq = 0;
    std::chrono::milliseconds step_time = 100ms;
    int ticks_last_time = 0;
    QueueHandle_t queue;
    SemaphoreHandle_t supervisor_semaphore;
    mot_pap* first_axis = nullptr;
    mot_pap* second_axis = nullptr;
    mot_pap* leader_axis = nullptr;
    class tmr tmr;
    volatile bool already_there = false;
    volatile bool was_soft_stopped = false;
    class kp kp;
    int error;

private:
    void calculate();

    bresenham(bresenham const&) = delete;
    void operator=(bresenham const&) = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

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
