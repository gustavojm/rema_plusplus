#ifndef DOUT_H_
#define DOUT_H_

#include <stdbool.h>

#include "mot_pap.h"

#ifdef __cplusplus
extern "C" {
#endif

void dout_init();

void dout_arm_dir(enum mot_pap_direction dir);

void dout_arm_pulse(void);

void dout_pole_dir(enum mot_pap_direction dir);

void dout_pole_pulse(void);

#ifdef __cplusplus
}
#endif

#endif /* DOUT_H_ */
