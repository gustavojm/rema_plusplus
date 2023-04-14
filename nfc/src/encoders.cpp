#include <stdint.h>
#include "board.h"
#include "mot_pap.h"
#include "encoders.h"
#include "gpio.h"

extern class mot_pap x_axis;
extern class mot_pap y_axis;
extern class mot_pap z_axis;

extern "C" void GPIO5_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(5));
    x_axis.update_position();
}

extern "C" void GPIO6_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(6));
    y_axis.update_position();
}

extern "C" void GPIO7_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(7));
    z_axis.update_position();
}


/**
 * @brief	Main program body
 * @return	Does not return
 */
void encoders_init(void) {
	//Chip_Clock_Enable(CLK_MX_GPIO);

    gpio {4, 0, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 0}.init_input();    //DIN0 P4_0     PIN01   GPIO2[0]   EncAx
    gpio {4, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 1}.init_input();    //DIN1 P4_1     PIN03   GPIO2[1]   EncAy
	gpio {4, 2, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 2}.init_input();    //DIN2 P4_2     PIN08   GPIO2[2]   EncAz

	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SCU_GPIOIntPinSel(5, 2, 0);
	Chip_SCU_GPIOIntPinSel(6, 2, 1);
	Chip_SCU_GPIOIntPinSel(7, 2, 2);

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
