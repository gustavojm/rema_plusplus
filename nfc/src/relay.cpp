#include "relay.h"
#include "board.h"
#include "gpio.h"

gpio relay_1 = { 2, 1, SCU_MODE_FUNC4, 5, 1 };	//DOUT0	P2_1	PIN81	GPIO5[1]
gpio relay_2 = { 4, 5, SCU_MODE_FUNC0, 2, 6 };	//DOUT1 P4_5	PIN10	GPIO2[5]
gpio relay_3 = { 4, 6, SCU_MODE_FUNC0, 2, 5 };	//DOUT2 P4_6 	PIN11	GPIO2[6]
gpio relay_4 = { 4, 4, SCU_MODE_FUNC0, 2, 4 };	//DOUT3	P4_4	PIN9	GPIO2[4]

/**
 * @brief 	initializes RELAYs
 * @returns	nothing
 * @note	outputs are set to low
 */
void relay_init() {
	relay_1.init_output();
	relay_1.set_pin_state(0);

	relay_2.init_output();
	relay_2.set_pin_state(0);

	relay_3.init_output();
	relay_3.set_pin_state(0);

	relay_4.init_output();
	relay_4.set_pin_state(0);
}

/**
 * @brief	sets GPIO corresponding to DOUT1 where MAIN_PWR relay is connected
 * @param 	state	: boolean value for the output
 * @returns	nothing
 */
void relay_main_pwr(bool state)
{
	relay_1.set_pin_state(state);
}

/**
 * @brief	sets GPIO corresponding to DOUT3 where SPARE relay is connected
 * @param 	state	: boolean value for the output
 * @returns	nothing
 */
void relay_spare_led(bool state)
{
	relay_4.set_pin_state(state);
}
