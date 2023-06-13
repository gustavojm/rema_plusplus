#ifndef TEMPERATURE_DS18B20_H_
#define TEMPERATURE_DS18B20_H_

#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"
#include "ds18b20.hpp"

void temperature_ds18b20_init();

uint32_t temperature_ds18b20_get(uint8_t sensor, float *var);

#endif /* TEMPERATURE_DS18B20_H_ */
