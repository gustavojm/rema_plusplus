#include <stdint.h>
#include "board.h"
#include "encoders.h"
#include "gpio.h"
#include "mot_pap.h"

/**
 * @brief	Main program body
 * @return	Does not return
 */
void x_zs_init(void)
{

	gpio {4, 2, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 2}.init_input();	//DIN2 P4_2 	PIN07  	GPIO2[2]   X_AXIS_POSITION_SENSOR


	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SCU_GPIOIntPinSel(2, 2, 2); // INT2 GPIO2[2]

	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(2));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(2));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(2));

	/* Enable interrupt in the NVIC */
	NVIC_ClearPendingIRQ(PIN_INT2_IRQn);
	NVIC_EnableIRQ(PIN_INT2_IRQn);


}
