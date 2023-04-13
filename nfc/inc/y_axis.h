#ifndef Y_AXIS_H_
#define Y_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap y_axis;

void y_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
static inline void TIMER2_IRQHandler(void) {
    if (y_axis.tmr.match_pending()) {
        y_axis.isr();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* Y_AXIS_H_ */
