#pragma once

#include "ds18b20.hpp"
#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"

inline volatile bool new_temps_available = false;

void temperature_ds18b20_init();

int16_t temperature_ds18b20_get(uint8_t sensor);
