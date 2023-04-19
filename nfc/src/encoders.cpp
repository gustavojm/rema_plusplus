#include <stdint.h>
#include "board.h"
#include "mot_pap.h"
#include "encoders.h"
#include "gpio.h"

extern class mot_pap x_axis;
extern class mot_pap y_axis;
extern class mot_pap z_axis;

extern "C" void GPIO0_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
    x_axis.update_position();
}

extern "C" void GPIO1_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(1));
    y_axis.update_position();
}

extern "C" void GPIO2_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(2));
    z_axis.update_position();
}


/**
 * @brief	Main program body
 * @return	Does not return
 */
void encoders_init(void) {
	//Chip_Clock_Enable(CLK_MX_GPIO);

    gpio_pinint encoders[] = {
            gpio_pinint {4, 0, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 0, PIN_INT0_IRQn},    //DIN0 P4_0     PIN01   GPIO2[0]   EncAx
            gpio_pinint {4, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 1, PIN_INT1_IRQn},    //DIN1 P4_1     PIN03   GPIO2[1]   EncAy
            gpio_pinint {4, 2, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 2, PIN_INT2_IRQn}     //DIN2 P4_2     PIN08   GPIO2[2]   EncAz
    };


    for (auto gpio_pin : encoders) {
        gpio_pin.init_input()
                .mode_edge()
                .int_low()
                .clear_pending()
                .enable();
    }
}
