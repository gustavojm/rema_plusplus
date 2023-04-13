#ifndef Z_AXIS_H_
#define Z_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap z_axis;

void z_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
static inline void TIMER3_IRQHandler(void) {
    if (z_axis.tmr.match_pending()) {
        z_axis.isr();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* Z_AXIS_H_ */
