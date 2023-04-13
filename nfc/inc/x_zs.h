#ifndef X_ZS_AXIS_H_
#define X_ZS_AXIS_H_

#include <stdint.h>
#include <stdbool.h>

#include "mot_pap.h"

void x_zs_init();

#ifdef __cplusplus
extern "C" {
#endif

void GPIO2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* X_ZS_AXIS_H_ */
