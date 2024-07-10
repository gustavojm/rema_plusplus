#ifndef BRESENHAM_H_
#define BRESENHAM_H_

#include <chrono>
#include <cstdint>
#include <string.h>

#include "FreeRTOS.h"
#include "debug.h"
#include "gpio.h"
#include "kp.h"
#include "mot_pap.h"
#include "semphr.h"
#include "task.h"
#include "tmr.h"

#define TASK_PRIORITY (configMAX_PRIORITIES - 3)
#define SUPERVISOR_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/**
 * @struct  bresenham_msg
 * @brief   messages to axis tasks.
 */
struct bresenham_msg {
  enum mot_pap::type type;
  int first_axis_setpoint;
  int second_axis_setpoint;
};

class bresenham {
public:
  bresenham() = delete;

  explicit bresenham(const char *name, mot_pap *first_axis,
                     mot_pap *second_axis, class tmr t, bool has_brakes = false)
      : name(name), first_axis(first_axis), second_axis(second_axis), tmr(t),
        has_brakes(has_brakes) {

    queue = xQueueCreate(5, sizeof(struct bresenham_msg *));
    supervisor_semaphore = xSemaphoreCreateBinary();

    char supervisor_task_name[configMAX_TASK_NAME_LEN];
    memset(supervisor_task_name, 0, sizeof(supervisor_task_name));
    strncat(supervisor_task_name, name,
            sizeof(supervisor_task_name) - strlen(supervisor_task_name) - 1);
    strncat(supervisor_task_name, "_supervisor",
            sizeof(supervisor_task_name) - strlen(supervisor_task_name) - 1);
    if (supervisor_semaphore != NULL) {
      // Create the 'handler' task, which is the task to which interrupt
      // processing is deferred
      xTaskCreate(
          [](void *axes) { static_cast<bresenham *>(axes)->supervise(); },
          supervisor_task_name, 256, this, SUPERVISOR_TASK_PRIORITY, &supervisor_task_handle);
      lDebug(Info, "%s: created", supervisor_task_name);
    }

    char task_name[configMAX_TASK_NAME_LEN];
    memset(task_name, 0, sizeof(task_name));
    strncat(task_name, name, sizeof(task_name) - strlen(task_name) - 1);
    strncat(task_name, "_task", sizeof(task_name) - strlen(task_name) - 1);
    xTaskCreate([](void *axes) { static_cast<bresenham *>(axes)->task(); },
                task_name, 256, this, TASK_PRIORITY, NULL);

    lDebug(Info, "%s: created", task_name);
  }

  void task();

  void set_timer(class tmr tmr) { this->tmr = tmr; }

  void supervise();

  void move(int first_axis_setpoint, int second_axis_setpoint);

  void step();

  void send(bresenham_msg msg);

  void isr();

  void stop();

  void pause();

  void resume();

public:
  const char *name;
  volatile bool is_moving = false;
  volatile int current_freq = 0;
  std::chrono::milliseconds step_time = std::chrono::milliseconds(100);
  TickType_t ticks_last_time = 0;
  QueueHandle_t queue;
  SemaphoreHandle_t supervisor_semaphore;
  TaskHandle_t supervisor_task_handle = nullptr;  
  mot_pap *first_axis = nullptr;
  mot_pap *second_axis = nullptr;
  mot_pap *leader_axis = nullptr;
  class tmr tmr;
  volatile bool already_there = false;
  volatile bool was_soft_stopped = false;
  volatile bool was_stopped_by_probe = false;
  volatile int touching_counter = 0;
  int touching_max_count = 3;
  bool has_brakes = false;
  class kp kp;
  volatile int error;

private:
  void calculate();

  bresenham(bresenham const &) = delete;
  void operator=(bresenham const &) = delete;

  // Note: Scott Meyers mentions in his Effective Modern
  //       C++ book, that deleted functions should generally
  //       be public as it results in better error messages
  //       due to the compilers behavior to check accessibility
  //       before deleted status
};

#endif /* BRESENHAM_H_ */
