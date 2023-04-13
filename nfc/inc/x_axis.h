#ifndef X_AXIS_H_
#define X_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap x_axis;

void x_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
static inline void TIMER1_IRQHandler(void) {
    if (x_axis.tmr.match_pending()) {
        x_axis.isr();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* X_AXIS_H_ */
