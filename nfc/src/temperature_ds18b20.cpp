#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"
#include "ds18b20.hpp"

#define TEMPERATURE_DS18B20_TASK_PRIORITY (configMAX_PRIORITIES - 4)

using gpio_ow = gpio_templ<2, 5, SCU_MODE_FUNC4 | SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP, 5, 5>;          // TODO asignar el pin correcto según arquitectura
using OneWireMaster = one_wire::BitBangOneWireMaster<gpio_ow>;

static void temperature_ds18b20_task(void *par) {

    OneWireMaster::connect();
    OneWireMaster::initialize();

    if (!OneWireMaster::touchReset()) {
        lDebug(Info, "No devices found!");
        vTaskDelay(pdMS_TO_TICKS(100));
        while (true) {
            // wait forever
        }
    }

    // search for connected DS18B20 devices
    OneWireMaster::resetSearch(0x28);

    uint8_t rom[3][8];
    int sens_index = 0;
    while (OneWireMaster::searchNext(rom[sens_index])) {
        char rom_str[8 * 3 + 1];
        char *cur_pos = rom_str;
        for (uint8_t i = 0; i < 8; i++) {
            cur_pos += sprintf(cur_pos, "%02X ", rom[sens_index][i]);
        }
        lDebug(Info, "found: %s", rom_str);
        sens_index++;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    lDebug(Info, "finished!");

    // read the temperature from a connected DS18B20
    Ds18b20< OneWireMaster > ds18b20(rom[0]);

    // 12 bits resolution => max. 750 ms conversion time
    ds18b20.configure(ds18b20::Resolution::Bits12);

    ds18b20.startConversion();

    while (true)
    {
        if (ds18b20.isConversionDone())
        {
            int16_t temperature = ds18b20.readTemperature();


            lDebug(Info, "Temperature: %d", temperature);
            vTaskDelay(pdMS_TO_TICKS(100));

            ds18b20.startConversion();
        }
    }
}

/**
 * @brief     initializes ADC to read temperature sensor.
 * @return    nothing
 */
void temperature_ds18b20_init() {
    xTaskCreate(temperature_ds18b20_task, "DS18B20",
    configMINIMAL_STACK_SIZE * 2, NULL,
    TEMPERATURE_DS18B20_TASK_PRIORITY, NULL);
    lDebug(Info, "DS18B20: task created");
}
