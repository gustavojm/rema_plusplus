#ifndef X_AXIS_H_
#define X_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap x_axis;

void x_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

void TIMER1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* X_AXIS_H_ */
