
#include <stdint.h>
#include "board.h"
#include "mot_pap.h"
#include "encoders.h"
#include "gpio.h"


int count_z = 0;
int count_b = 0;
int count_a = 0;

extern class mot_pap x_axis;
extern class mot_pap y_axis;
extern class mot_pap z_axis;

/**
* @brief	Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return	Nothing
*/
/**
* @brief	Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return	Nothing
*/
void GPIO5_IRQHandler(void)
{
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(5));
	x_axis.update_position();
}

void GPIO6_IRQHandler(void)
{
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(6));
	++count_b;
}

void GPIO7_IRQHandler(void)
{
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(7));
	++count_z;
}

/**
 * @brief	Main program body
 * @return	Does not return
 */
void encoders_init(void) {
	//Chip_Clock_Enable(CLK_MX_GPIO);
	count_z = 0;
	count_b = 0;
	count_a = 0;

	gpio {7, 4, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 12}.init_input();
	gpio {7, 5, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 13}.init_input();
	gpio {7, 6, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 14}.init_input();

	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SCU_GPIOIntPinSel(5, 3, 12);
	Chip_SCU_GPIOIntPinSel(6, 3, 13);
	Chip_SCU_GPIOIntPinSel(7, 3, 14);

	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(5));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(5));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(5));
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(6));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(6));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(6));
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(7));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(7));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(7));

	/* Enable interrupt in the NVIC */
	NVIC_ClearPendingIRQ(PIN_INT5_IRQn);
	NVIC_EnableIRQ(PIN_INT5_IRQn);
	NVIC_ClearPendingIRQ(PIN_INT6_IRQn);
	NVIC_EnableIRQ(PIN_INT6_IRQn);
	NVIC_ClearPendingIRQ(PIN_INT7_IRQn);
	NVIC_EnableIRQ(PIN_INT7_IRQn);

}
