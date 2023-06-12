#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "gpio_templ.h"
#include "one-wire_bitbang_master.hpp"
#include "ds18b20.hpp"

using gpio_ow = gpio_templ<4, 10, SCU_MODE_FUNC4, 5, 14>;      // TODO asignar bien el pin correcto

const uint8_t rom_1[] = {0xDE, 0xAD, 0xBE, 0xEF};

Ds18b20<one_wire::BitBangOneWireMaster<gpio_ow>> sens_1(rom_1);  // TODO poner ROM ADDRESS EN CONSTRUCTOR
Ds18b20<one_wire::BitBangOneWireMaster<gpio_ow>> sens_2(rom_1);  // TODO poner ROM ADDRESS EN CONSTRUCTOR
Ds18b20<one_wire::BitBangOneWireMaster<gpio_ow>> sens_3(rom_1);  // TODO poner ROM ADDRESS EN CONSTRUCTOR

#define TEMPERATURE_DS18B20_TASK_PRIORITY (configMAX_PRIORITIES - 4)

/**
 * @brief    returns status temperature of the NFC.
 * @return     the temperature value in °C
 */



static int temperature_ds18b20_read() {
    sens_1.configure(ds18b20::Resolution::Bits12);

    return 1;
}

static void temperature_ds18b20_task(void *par) {
    while (true) {
//        temperature_ds18b20_read(0, &temperatures[0]);
//        temperature_ds18b20_read(1, &temperatures[1]);

        temperature_ds18b20_read();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief     initializes ADC to read temperature sensor.
 * @return    nothing
 */
void temperature_ds18b20_init() {
    //one_wire_init();
    //ds18b20_init();
    //uint8_t err = ds18b20_search_and_assign_ROM_codes();
    //lDebug(Info, "search_And_assign_ROM: %i", err);
    // lDebug(Info, "read_ROM: %i", ds18b20_read_ROM(0));
    xTaskCreate(temperature_ds18b20_task, "DS18B20",
    configMINIMAL_STACK_SIZE * 2, NULL,
    TEMPERATURE_DS18B20_TASK_PRIORITY, NULL);
    lDebug(Info, "DS18B20: task created");
}
