#ifndef MOT_PAP_H_
#define MOT_PAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOT_PAP_MAX_FREQ                        125000
#define MOT_PAP_MIN_FREQ                        100
#define MOT_PAP_CLOSED_LOOP_FREQ_MULTIPLIER     (MOT_PAP_MAX_FREQ / 100)
#define MOT_PAP_MAX_SPEED_FREE_RUN              8
#define MOT_PAP_COMPUMOTOR_MAX_FREQ             300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS       500

#define MOT_PAP_SUPERVISOR_RATE                 3000  // 2 means one step
#define MOT_PAP_POS_PROXIMITY_THRESHOLD         100
#define MOT_PAP_POS_THRESHOLD                   6
#define MOT_PAP_STALL_THRESHOLD                 3
#define MOT_PAP_STALL_MAX_COUNT                 40

extern bool stall_detection;

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
		MOT_PAP_DIRECTION_CW, MOT_PAP_DIRECTION_CCW,
	};

	enum type {
		MOT_PAP_TYPE_FREE_RUNNING, MOT_PAP_TYPE_CLOSED_LOOP, MOT_PAP_TYPE_STEPS, MOT_PAP_TYPE_STOP
	};

	/**
	 * @struct 	mot_pap_gpios
	 * @brief	pointers to functions to handle GPIO lines of this stepper motor.
	 */
	struct gpios {
		struct gpio_entry direction;
		struct gpio_entry step;
	};

	explicit mot_pap(const char *name, tmr t);

	void set_offset(uint16_t offset) {
		this->offset = offset;
	}

	void set_timer(class tmr tmr) {
		this->tmr = tmr;
	}

	void set_gpios(struct gpios gpios) {
		this->gpios = gpios;
	}

	uint16_t offset_correction(uint16_t pos, uint16_t offset);

	void read_corrected_pos();

	void supervise();

	void new_cmd_received();

	void move_free_run(enum direction direction, uint32_t speed);

	void move_steps(enum direction direction,
			uint32_t speed, uint32_t steps);

	void move_closed_loop(uint16_t setpoint);

	static void init();

	void stop();

	void isr();

	void update_position();

	enum mot_pap::direction direction_calculate(int32_t error) const;

	bool free_run_speed_ok(uint32_t speed) const;

	uint32_t read_on_condition(void);

	JSON_Value* json() const;

public:
	static const uint32_t free_run_freqs[];
	const char *name;
	enum type type;
	enum direction dir;
	int32_t posAct;
	int32_t posCmd;
	int32_t posCmdMiddle;
	int32_t requested_freq;
	int32_t freq_increment;
	int32_t current_freq;
	int32_t last_pos;
	uint32_t stalled_counter;
	struct gpios gpios;
	class tmr tmr;
	enum direction last_dir;
	int32_t half_pulses;// counts steps from the last call to supervisor task
	int32_t offset;
	int32_t half_steps_requested;
	int32_t half_steps_curr;
	int32_t half_steps_to_middle;
	int32_t max_speed_reached_distance;
	int32_t ticks_last_time;
	bool max_speed_reached;
	bool already_there;
	bool stalled;
};

#endif /* MOT_PAP_H_ */
