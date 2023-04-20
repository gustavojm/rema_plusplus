#include <stdint.h>
#include "board.h"
#include "gpio.h"
#include "mot_pap.h"

extern mot_pap x_axis;
extern mot_pap y_axis;
extern mot_pap z_axis;

static void hard_limits_reached() {
    /* TODO Read input pins to determine which limit has been reached and stop only one motor*/
    x_axis.stop();
    y_axis.stop();
    z_axis.stop();
}

/**
* @brief    Handle Grouped Interrupt from GPIO pin or GPIO pin mapped to PININT
* @return   Nothing
*/
extern "C" void GINT0_IRQHandler(void)
{
    hard_limits_reached();
    Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 0);
}

extern "C" void GINT1_IRQHandler(void)
{
    hard_limits_reached();
    Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 1);
}


void hard_limits_init(void)
{
    gpio ZSz_in = gpio {4, 3, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 3}.init_input();       //DIN3 P4_3     PIN07   GPIO2[3]   ZSz+

    /* Group GPIO interrupt 0 */
    Chip_GPIOGP_SelectLowLevel(LPC_GPIOGROUP, 0, ZSz_in.gpio_port, 1 << ZSz_in.gpio_bit);
    Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 0, ZSz_in.gpio_port, 1 << ZSz_in.gpio_bit);
    Chip_GPIOGP_SelectOrMode(LPC_GPIOGROUP, 0);
    Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 0);

    /* Enable Group GPIO interrupt 0 */
    //NVIC_EnableIRQ(GINT0_IRQn);

    /* Group GPIO interrupt 1 */
    gpio ZSz_out   = gpio {7, 3, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 11}.init_input();    //DIN4 P7_3     PIN117   GPIO3[11]   ZSz-
    gpio ZSy_up    = gpio {7, 4, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 12}.init_input();    //DIN4 P7_4     PIN132   GPIO3[12]   ZSy+
    gpio ZSy_down  = gpio {7, 5, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 13}.init_input();    //DIN4 P7_5     PIN133   GPIO3[13]   ZSy-
    gpio ZSx_right = gpio {7, 6, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 14}.init_input();    //DIN4 P7_6     PIN134   GPIO3[14]   ZSx+
    gpio ZSx_left  = gpio {7, 0, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 3, 8}.init_input();     //DIN4 P7_0     PIN110   GPIO3[08]   ZSx-

    int gpio_port = 3;
    int bitmask = 1 << ZSz_out.gpio_bit | 1 << ZSy_up.gpio_bit | 1 << ZSy_down.gpio_bit | 1 << ZSx_right.gpio_bit | 1 << ZSx_left.gpio_bit;

    /* Group GPIO interrupt 1 will be invoked when the button is pressed. */
    Chip_GPIOGP_SelectLowLevel(LPC_GPIOGROUP, 1, gpio_port, bitmask);
    Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 1, gpio_port, bitmask);
    Chip_GPIOGP_SelectOrMode(LPC_GPIOGROUP, 1);
    Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 1);

    /* Enable Group GPIO interrupt 1 */
    //NVIC_EnableIRQ(GINT1_IRQn);
}
