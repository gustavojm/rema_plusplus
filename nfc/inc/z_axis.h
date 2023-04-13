#ifndef Z_AXIS_H_
#define Z_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap z_axis;

void z_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

void TIMER3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* Z_AXIS_H_ */
