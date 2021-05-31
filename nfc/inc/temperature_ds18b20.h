#ifndef TEMPERATURE_DS18B20_H_
#define TEMPERATURE_DS18B20_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void temperature_ds18b20_init();

uint32_t temperature_ds18b20_get(uint8_t sensor, float *var);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_DS18B20_H_ */
