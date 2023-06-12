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


#define MOT_PAP_MAX_FREQ                        500000
#define MOT_PAP_COMPUMOTOR_MAX_FREQ             500000 //300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS       500

#define MOT_PAP_POS_THRESHOLD                   1
#define MOT_PAP_STALL_THRESHOLD                 3
#define MOT_PAP_STALL_MAX_COUNT                 5

using namespace std::chrono_literals;
/**
 * @struct 	mot_pap
 * @brief	axis structure.
 */
class mot_pap {
public:
	enum direction {
		DIRECTION_CW, DIRECTION_CCW, DIRECTION_NONE,
	};

	enum type {
		TYPE_FREE_RUNNING, TYPE_CLOSED_LOOP, TYPE_STOP, TYPE_BRESENHAM
	};

    /**
     * @struct  mot_pap_gpios
     * @brief   pointers to functions to handle GPIO lines of this stepper motor.
     */
    struct gpios {
        gpio direction;
        gpio step;
    };

	mot_pap() = delete;

    explicit mot_pap(const char *name, bool is_dummy = true) :
            name(name), is_dummy(is_dummy){
    }

	explicit mot_pap(const char *name, gpio direction_pin, gpio step_pin, bool is_dummy = false) :
	        name(name), is_dummy(is_dummy){
	}

    enum direction direction_calculate(int error);

	void set_position(double pos)
	{
	    current_counts() = static_cast<int>(pos * inches_to_counts_factor);
	}

	void set_direction(enum direction direction);

	void set_direction();

	void step();

	void update_position();

	void update_position_simulated();

	bool check_for_stall();

	bool check_already_there();

	void soft_stop(int counts);

	JSON_Value* json() const;

public:
	const char *name;
	enum type type = TYPE_STOP;
	enum direction dir = DIRECTION_NONE;
    bool probe_triggered = false;
    int probe_pos = 0;
    enum direction probe_last_dir = DIRECTION_NONE;
	int last_pos = 0;
	int inches_to_counts_factor = 0;
	int motor_resolution = 0;
	int encoder_resolution = 0;
	int stalled_counter = 0;
	int delta = 0;
    struct gpios gpios;
	enum direction last_dir = DIRECTION_NONE;
	unsigned int half_pulses_stall = 0;                // counts steps from the last call to stall control
	unsigned int half_pulses = 0;                      // counts steps for encoder simulation
	bool already_there = false;
	bool stalled = false;
    bool reversed = false;

    volatile int&  current_counts()        { return current_counts_; }  // setter
    volatile const int& current_counts() const  { return current_counts_; } // getter

    int&  destination_counts()        { return destination_counts_; }
    const int& destination_counts() const  { return destination_counts_; }

private:
    volatile int current_counts_ = 0;
    int destination_counts_ = 0;
    bool is_dummy;
};

#endif /* MOT_PAP_H_ */
