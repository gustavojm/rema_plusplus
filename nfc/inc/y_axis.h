#ifndef Y_AXIS_H_
#define Y_AXIS_H_

#include <stdint.h>

#include "mot_pap.h"

extern mot_pap y_axis;

void y_axis_init();

#ifdef __cplusplus
extern "C" {
#endif

void TIMER2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* Y_AXIS_H_ */
