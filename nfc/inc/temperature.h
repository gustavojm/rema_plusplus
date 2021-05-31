#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void temperature_init();

uint16_t temperature_read(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_H_ */
