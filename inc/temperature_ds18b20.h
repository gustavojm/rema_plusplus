#ifndef TEMPERATURE_DS18B20_H_
#define TEMPERATURE_DS18B20_H_

#include "ds18b20.hpp"
#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"

void temperature_ds18b20_init();

int16_t temperature_ds18b20_get(uint8_t sensor);

#endif /* TEMPERATURE_DS18B20_H_ */
