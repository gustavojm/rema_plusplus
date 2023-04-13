#ifndef X_ZS_AXIS_H_
#define X_ZS_AXIS_H_

#include <stdint.h>
#include <stdbool.h>

#include "mot_pap.h"

void x_zs_init();

extern mot_pap x_axis;

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief    Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return   Nothing
*/
static inline void GPIO2_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(2));
    x_axis.stop();
}

#ifdef __cplusplus
}
#endif

#endif /* X_ZS_AXIS_H_ */
