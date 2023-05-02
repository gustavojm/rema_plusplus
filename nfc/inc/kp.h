#ifndef KP_H_
#define KP_H_

#include <chrono>

//! @brief 		Error proportional -kp- control for frequency output.
class kp {
public:

    //! @brief      Enumerates the controller direction modes
    enum controller_direction {
        DIRECT,          //!< Direct drive (+error gives +output)
        REVERSE          //!< Reverse driver (+error gives -output)
    };

	//! @brief 		The set-point the error proportional -kp- control is trying to make the output converge to.
	int setpoint;

	//! @brief		The control output.
	//! @details	This is updated when kp::run() is called.
	int output;

	//! @brief		Proportional constant
	float kp_;

	//! @brief		The sample period (in milliseconds) between successive kp::run() calls.
	std::chrono::milliseconds sample_period_ms;

	float p_term = 0;		//!< The proportional term that is summed as part of the output (calculated in Pid_Run())
	int out_min;	//!< The minimum output value. Anything lower will be limited to this floor.
	int out_max;	//!< The maximum output value. Anything higher will be limited to this ceiling.
	int out_abs_min;	//!< The absolute minimum output value. Anything lower will be limited to this floor.

	//! @brief		Counts the number of times that Run() has be called. Used to stop
	//!				derivative control from influencing the output on the first call.
	//! @details	Safely stops counting once it reaches 2^32-1 (rather than overflowing).
	int num_times_ran = 0;

	//! @brief		The controller direction (FORWARD or REVERSE).
	enum controller_direction controller_dir;

    //! @brief      Constructor
    kp() = default;


    //! @brief 		Constructor
    //! @details   	The parameters specified here are those for for which we can't set up
    //!    			reliable defaults, so we need to have the user set them.
    kp(int kp,
            enum controller_direction controller_dir,
            std::chrono::milliseconds sample_period_ms, int min_output, int max_output, int min_abs_output);

    void restart();

    //! @brief 		Computes new KP values
    //! @details 	Call once per sampleTimeMs. Output is stored in the kpData structure.
    int run(int setpoint, int input);

    void set_output_limits(int min, int max, int min_abs_output);

    //! @details	The KP will either be connected to a direct acting process (+error leads to +output, aka inputs are positive)
    //!				or a reverse acting process (+error leads to -output, aka inputs are negative)
    void set_controller_direction(enum controller_direction controller_dir);

    //! @brief		Changes the sample time
    void set_sample_period(std::chrono::milliseconds new_sample_period_ms);

    //! @brief		This function allows the controller's dynamic performance to be adjusted.
    //! @details	It's called automatically from the init function, but tunings can also
    //! 			be adjusted on the fly during normal operation
    void set_tunings(float kp);

    //! @brief		Returns the actual (not time-scaled) proportional constant.
    int get_kp();

};

#endif /* KP_H_ */
