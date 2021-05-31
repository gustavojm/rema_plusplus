#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "one-wire.h"
#include "ds18b20.h"
#include "temperature_ds18b20.h"

#define STB_DEFINE

#include "misc.h"

#define TEMPERATURE_DS18B20_TASK_PRIORITY ( configMAX_PRIORITIES - 4 )

static float temperatures[2];

uint32_t temperature_ds18b20_get(uint8_t sensor, float *var) {
	if (sensor >= (sizeof temperatures / sizeof temperatures[0])) {
		return ERR_RANGE; /* error */
	}
	*var = temperatures[sensor];
	return ERR_OK;
}

/**
 * @brief	returns status temperature of the NFC.
 * @return 	the temperature value in °C
 */
int temperature_ds18b20_read(uint8_t sensor, float *var)
{
	int err;
	if (sensor >= (sizeof temperatures / sizeof temperatures[0])) {
		return ERR_RANGE; /* error */
	}

	err = ds18b20_start_conversion(sensor);
	lDebug(Info, "start_conversion: %i", err);
	if (err != ERR_OK) {
		return err;
	}

	err = ds18b20_read_temperature(sensor);
	lDebug(Info, "read_temperature: %i", err);
	if (err != ERR_OK) {
		return err;
	}

	err = ds18b20_get_temperature_float(sensor, var);
	lDebug(Info, "get_tempeature_float: %i", err);
	if (err != ERR_OK) {
		return err;
	}

//	char buff[6];
//	snprint_double(buff, sizeof buff / sizeof buff[0], temp, 2);
//	lDebug(Info, "Temperatura leida del DS18B20: %s", buff);

	return ERR_OK;

}

static void temperature_ds18b20_task(void *par)
{
	while (true) {

		temperature_ds18b20_read(0, &temperatures[0]);
		temperature_ds18b20_read(1, &temperatures[1]);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/**
 * @brief 	initializes ADC to read temperature sensor.
 * @return	nothing
 */
void temperature_ds18b20_init()
{
	one_wire_init();
	ds18b20_init();
	uint8_t err = ds18b20_search_and_assign_ROM_codes();
	lDebug(Info, "search_And_assign_ROM: %i", err);
	//lDebug(Info, "read_ROM: %i", ds18b20_read_ROM(0));
	xTaskCreate(temperature_ds18b20_task, "DS18B20", configMINIMAL_STACK_SIZE * 2, NULL,
			TEMPERATURE_DS18B20_TASK_PRIORITY, NULL);
	lDebug(Info, "DS18B20: task created");
}
