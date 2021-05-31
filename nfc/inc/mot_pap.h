#ifndef MOT_PAP_H_
#define MOT_PAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"
#include "ad2s1210.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOT_PAP_MAX_FREQ						125000
#define MOT_PAP_MIN_FREQ						100
#define MOT_PAP_CLOSED_LOOP_FREQ_MULTIPLIER  	( MOT_PAP_MAX_FREQ / 100 )
#define MOT_PAP_MAX_SPEED_FREE_RUN				8
#define MOT_PAP_COMPUMOTOR_MAX_FREQ				300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS		500

#define MOT_PAP_SUPERVISOR_RATE    				3000	//2 means one step
#define MOT_PAP_POS_PROXIMITY_THRESHOLD			100
#define MOT_PAP_POS_THRESHOLD 					6
#define MOT_PAP_STALL_THRESHOLD 				3
#define MOT_PAP_STALL_MAX_COUNT					40

extern bool stall_detection;


#ifdef __cplusplus
}
#endif

/**
 * @struct 	mot_pap
 * @brief	POLE or ARM structure.
 */
class mot_pap {
public:
	enum direction {
		MOT_PAP_DIRECTION_CW, MOT_PAP_DIRECTION_CCW,
	};

	enum type {
		MOT_PAP_TYPE_FREE_RUNNING, MOT_PAP_TYPE_CLOSED_LOOP, MOT_PAP_TYPE_STOP
	};

	/**
	 * @struct 	mot_pap_msg
	 * @brief	messages to POLE or ARM tasks.
	 */
	struct msg {
		enum type type;
		enum direction free_run_direction;
		uint32_t free_run_speed;
		uint16_t closed_loop_setpoint;
	};

	/**
	 * @struct 	mot_pap_gpios
	 * @brief	pointers to functions to handle GPIO lines of this stepper motor.
	 */
	struct gpios {
		void (*direction)(enum direction dir); ///< pointer to direction line function handler
		void (*pulse)(void);			///< pointer to pulse line function handler
	};


	const char *name;
	enum type type;
	enum direction dir;
	uint16_t posCmd;
	uint16_t posAct;
	uint32_t freq;
	bool stalled;
	bool already_there;
	uint16_t stalled_counter;
	ad2s1210 *rdc;
	SemaphoreHandle_t supervisor_semaphore;
	struct gpios gpios;
	class tmr *tmr;
	enum direction last_dir;
	uint16_t last_pos;
	uint32_t half_pulses;// counts steps from the last call to supervisor task
	uint16_t offset;

	mot_pap(const char *name);

	void set_offset(uint16_t offset)
	{
		this->offset = offset;
	}

	void set_rdc(ad2s1210 *rdc)
	{
		this->rdc = rdc;
	}

	void set_timer(class tmr *tmr)
	{
		this->tmr = tmr;
	}

	void set_gpios(struct gpios gpios)
	{
		this->gpios = gpios;
	}

	void set_supervisor_semaphore (SemaphoreHandle_t supervisor_semaphore) {
		this->supervisor_semaphore = supervisor_semaphore;
	}

	uint16_t offset_correction(uint16_t pos, uint16_t offset,
			uint8_t resolution);

	void read_corrected_pos();

	void supervise();

	void new_cmd_received();

	void move_free_run(enum direction direction, uint32_t speed);

	void move_closed_loop(uint16_t setpoint);

	void stop();

	void isr();

	void update_position();

	enum mot_pap::direction direction_calculate(int32_t error);

	bool free_run_speed_ok(uint32_t speed);

	uint32_t read_on_condition(void);

	JSON_Value* json();
};

#endif /* MOT_PAP_H_ */
