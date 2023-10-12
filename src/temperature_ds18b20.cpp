#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"
#include "ds18b20.hpp"

#define TEMPERATURE_DS18B20_TASK_PRIORITY (configMAX_PRIORITIES - 4)

using gpio_ow = gpio_templ<2, 5, SCU_MODE_FUNC4 | SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP, 5, 5>;          // TODO asignar el pin correcto según arquitectura
using OneWireMaster = one_wire::BitBangOneWireMaster<gpio_ow>;
constexpr int reading_interval = 1000;
constexpr int max_sensors = 3;

struct sensor {
    Ds18b20<OneWireMaster>* ds18b20;
    int16_t reading;
};

sensor sensors[max_sensors];
int detected_sensors = 0;

static void temperature_ds18b20_task(void *par) {

    OneWireMaster::connect();
    OneWireMaster::initialize();

    if (!OneWireMaster::touchReset()) {
        lDebug(Info, "No devices found! Suspending task");
        vTaskSuspend(NULL);
    }

    // search for connected DS18B20 devices
    OneWireMaster::resetSearch(0x28);

    uint8_t rom[8];
    while (OneWireMaster::searchNext(rom)) {
        char rom_str[8 * 3 + 1];
        char *cur_pos = rom_str;
        for (uint8_t i = 0; i < 8; i++) {
            cur_pos += sprintf(cur_pos, "%02X ", rom[i]);
        }
        lDebug(Info, "found: %s", rom_str);
        sensors[detected_sensors].ds18b20 = new Ds18b20<OneWireMaster>(rom);

        detected_sensors++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    lDebug(Info, "Sensor detection finished, %i found", detected_sensors);
    lDebug(Info, "Updating readings every %ims", reading_interval);


    // read the temperature from a connected DS18B20 sensor

    sensors[0].ds18b20->startConversions();

    while (true) {
        for (int i=0; i<detected_sensors; i++) {
            if (sensors[i].ds18b20->isConversionDone()) {
                sensors[i].reading = sensors[i].ds18b20->readTemperature();
                vTaskDelay(pdMS_TO_TICKS(reading_interval));

                sensors[0].ds18b20->startConversions();
            }
        }
    }
}


int16_t temperature_ds18b20_get(uint8_t sensor) {
    if (sensor > detected_sensors) {
        return INT16_MIN;
    }
    return sensors[sensor].reading;
}


/**
 * @brief     initializes DS18B20 sensors to read temperatures.
 * @return    nothing
 */
void temperature_ds18b20_init() {
    xTaskCreate(temperature_ds18b20_task, "DS18B20",
    configMINIMAL_STACK_SIZE * 2, NULL,
    TEMPERATURE_DS18B20_TASK_PRIORITY, NULL);
    lDebug(Info, "DS18B20: task created");
}
