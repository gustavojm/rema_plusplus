#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "temperature.h"

#define LPC_ADC LPC_ADC0
#define TEMP_ADC_CH ADC_CH1
#define TEMPERATURE_COUNT 200
#define ADC_SAMPLE_RATE 1000

/**
 * @brief 	initializes ADC to read temperature sensor.
 * @return	nothing
 */
void temperature_init()
{
	ADC_CLOCK_SETUP_T adc_setup;

	Chip_ADC_Init(LPC_ADC, &adc_setup);
	Chip_ADC_SetSampleRate(LPC_ADC, &adc_setup, ADC_SAMPLE_RATE);
	Chip_ADC_EnableChannel(LPC_ADC, TEMP_ADC_CH, ENABLE);
	Chip_ADC_SetBurstCmd(LPC_ADC, ENABLE);
}

/**
 * @brief	returns status of the temperature task.
 * @return 	the temperature value in °C
 */
uint16_t temperature_read(void)
{
	static uint16_t temperature = 0;
	static uint16_t actual_temperature = 0;
	static uint16_t aTemperature[TEMPERATURE_COUNT] = { };
	static uint16_t i = 0;

	uint16_t sum = 0;

	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, TEMP_ADC_CH, ADC_DR_DONE_STAT) != SET) {
	}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, TEMP_ADC_CH, &actual_temperature);
	aTemperature[i] = actual_temperature;

	++i;

	if (i == TEMPERATURE_COUNT) {
		for (uint16_t x = 0; x < TEMPERATURE_COUNT; x++) {
			sum += aTemperature[x];
		}

		temperature = sum / TEMPERATURE_COUNT;
		i = 0;
	}
	/* Print ADC value */

	return temperature;

}
