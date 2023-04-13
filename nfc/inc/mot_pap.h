#ifndef MOT_PAP_H_
#define MOT_PAP_H_

#include <stdint.h>
#include <chrono>

#include "gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"
#include "gpio.h"
#include "kp.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MOT_PAP_MAX_FREQ                        125000
#define MOT_PAP_COMPUMOTOR_MAX_FREQ             300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS       500

#define MOT_PAP_SUPERVISOR_RATE                 625  // 2 means one step
#define MOT_PAP_POS_THRESHOLD                   2
#define MOT_PAP_STALL_THRESHOLD                 3
#define MOT_PAP_STALL_MAX_COUNT                 40

#ifdef __cplusplus
}
#endif

/**
 * @struct 	mot_pap
 * @brief	axis structure.
 */
class mot_pap {
public:
	enum direction {
		DIRECTION_CW, DIRECTION_CCW,
	};

	enum type {
		TYPE_FREE_RUNNING, TYPE_CLOSED_LOOP, TYPE_STOP
	};

	/**
	 * @struct 	mot_pap_gpios
	 * @brief	pointers to functions to handle GPIO lines of this stepper motor.
	 */
	struct gpios {
	    gpio direction;
		gpio step;
	};

	mot_pap() = delete;

	void task();

	explicit mot_pap(const char *name, tmr t);

	void set_offset(int offset) {
		this->offset = offset;
	}

	void set_position(double pos)
	{
	    pos_act = static_cast<int>(pos) * inches_to_counts_factor;
	}

	void set_timer(class tmr tmr) {
		this->tmr = tmr;
	}

	void set_gpios(struct gpios gpios) {
		this->gpios = gpios;
	}

	void read_corrected_pos();

	void supervise();

	void new_cmd_received();

	void move_free_run(enum direction direction, int speed);

	void move_closed_loop(int setpoint);

	void stop();

	void isr();

	void update_position();

	enum mot_pap::direction direction_calculate(int32_t error) const;

	bool free_run_speed_ok(uint32_t speed) const;

	uint32_t read_on_condition(void);

	JSON_Value* json() const;

public:
	const char *name;
	enum type type;
	enum direction dir;
    volatile int pos_act;
    int pos_cmd;
	int32_t requested_freq;
	int32_t freq_increment;
	int32_t current_freq;
    std::chrono::milliseconds step_time;
	int last_pos;
	int inches_to_counts_factor;
	uint32_t stalled_counter;
	struct gpios gpios;
	enum direction last_dir;
	int half_pulses;// counts steps from the last call to supervisor task
	int offset;
	class tmr tmr;
	int ticks_last_time;
	bool already_there;
	bool stalled;
    QueueHandle_t queue;
    SemaphoreHandle_t supervisor_semaphore;
    class kp kp;

};

/**
 * @struct  mot_pap_msg
 * @brief   messages to axis tasks.
 */
struct mot_pap_msg {
    enum mot_pap::type type;
    enum mot_pap::direction free_run_direction;
    int free_run_speed;
    float closed_loop_setpoint;
    int steps;
    struct mot_pap *axis;
};

#endif /* MOT_PAP_H_ */
