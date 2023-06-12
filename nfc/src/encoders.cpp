#include <stdint.h>
#include "board.h"
#include "mot_pap.h"
#include "encoders.h"
#include "gpio.h"

extern class mot_pap x_axis, y_axis, z_axis;

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

void encoders_init(void) {
	//Chip_Clock_Enable(CLK_MX_GPIO);

    gpio_pinint encoders[] = {
            gpio_pinint {2, 5, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC4), 5, 5, PIN_INT0_IRQn},    //GPIO1 P2_5     PIN91   GPIO5[5]    EncAx
            gpio_pinint {7, 0, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 8, PIN_INT1_IRQn},    //GPIO2 P7_0     PIN110  GPIO3[8]    EncAy
            gpio_pinint {6, 7, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC4), 5, 15, PIN_INT2_IRQn}    //GPIO3 P6_7     PIN85   GPIO5[15]   EncAz
    };


    for (auto gpio_pin : encoders) {
        gpio_pin.init_input()
                .mode_edge()
                .int_low()
                .clear_pending()
                .enable();
    }
}
