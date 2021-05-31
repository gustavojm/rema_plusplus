#ifndef RELAY_H_
#define RELAY_H_

#include <stdbool.h>

#include "lift.h"

#ifdef __cplusplus
extern "C" {
#endif

void relay_init();

void relay_lift_dir(enum lift_direction dir);

void relay_lift_pwr(bool state);

void relay_main_pwr(bool state);

void relay_spare_led(bool state);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_H_ */
