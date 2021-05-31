#include <stdbool.h>

#include "board.h"
#include "dout.h"
#include "mot_pap.h"

/**
 * @brief 	initializes DOUTs
 * @returns	nothing
 */
void dout_init()
{
	Chip_SCU_PinMuxSet( 4, 8, SCU_MODE_FUNC4 );			//DOUT4 P4_8 	PIN15  	GPIO5[12]   ARM_DIR
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 12);

	Chip_SCU_PinMuxSet( 4, 9, SCU_MODE_FUNC4 );			//DOUT5 P4_9  	PIN33	GPIO5[13] 	ARM_PULSE
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 13);

	Chip_SCU_PinMuxSet( 4, 10, SCU_MODE_FUNC4 );		//DOUT6 P4_10 	PIN35	GPIO5[14] 	POLE_DIR
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 14);

	Chip_SCU_PinMuxSet( 1, 5, SCU_MODE_FUNC0 );			//DOUT7 P1_5 	PIN48 	GPIO1[8]   	POLE_PULSE
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 1, 8);
}

/**
 * @brief	sets GPIO corresponding to DOUT4 where ARM_DIR is connected
 * @param 	dir		: direction of movement. Should be:
 * 					  MOT_PAP_DIRECTION_CW
 * 					  MOT_PAP_DIRECTION_CCW
 * @returns	nothing
 */
void dout_arm_dir(enum mot_pap_direction dir)
{
	if (dir == MOT_PAP_DIRECTION_CW) {
		Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 12);
	} else {
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 12);
	}
}

/**
 * @brief	toggles GPIO corresponding to DOUT5 where ARM_PULSE is connected
 * @returns nothing
 */
void dout_arm_pulse(void)
{
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, 5, 13);
}

/**
 * @brief	sets GPIO corresponding to DOUT6 where POLE_DIR is connected
 * @param 	dir		: direction of movement. Should be:
 * 					  MOT_PAP_DIRECTION_CW
 * 					  MOT_PAP_DIRECTION_CCW
 * @returns	nothing
 */
void dout_pole_dir(enum mot_pap_direction dir)
{
	if (dir == MOT_PAP_DIRECTION_CW) {
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 14);
	} else {
		Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 14);
	}
}

/**
 * @brief	toggles GPIO corresponding to DOUT7 where POLE_PULSE is connected
 * @returns nothing
 */
void dout_pole_pulse(void)
{
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, 1, 8);
}
