#ifndef ENCODERS_H_
#define ENCODERS_H_

#include "mot_pap.h"

void encoders_init();

extern class mot_pap x_axis;
extern class mot_pap y_axis;
extern class mot_pap z_axis;

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief    Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return   Nothing
*/
static inline void GPIO5_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(5));
    x_axis.update_position();
}

static inline void GPIO6_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(6));
    y_axis.update_position();
}

static inline void GPIO7_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(7));
    z_axis.update_position();
}

#ifdef __cplusplus
}
#endif

#endif /* ENCODERS_H_ */


