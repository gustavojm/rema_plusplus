#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* sensor resolution */
typedef enum {
  DS18B20_RESOLUTION_BITS_9  = 0b00, /* conversion time: 93.75 ms */
  DS18B20_RESOLUTION_BITS_10 = 0b01, /* conversion time: 187.5 ms */
  DS18B20_RESOLUTION_BITS_11 = 0b10, /* conversion time: 375 ms */
  DS18B20_RESOLUTION_BITS_12 = 0b11  /* conversion time: 750 ms */
} DS18B20_ResolutionBits;

#define DS18B20_FAMILY_CODE    0x28  /* 8-bit family code for DS128B20 */
#define DS18B20_ROM_CODE_SIZE  8     /* 8 byte ROM code (family ID, 6 bytes for ID plus 1 byte CRC */

uint8_t ds18b20_start_conversion(uint8_t sensor_index);

uint8_t ds18b20_set_resolution(uint8_t sensor_index, DS18B20_ResolutionBits resolution);

uint8_t ds18b20_read_temperature(uint8_t sensor_index);

uint8_t ds18b20_get_temperature_raw(uint16_t sensor_index, uint32_t *raw);

bool ds18b20_is_busy(void);

uint8_t ds18b20_get_ROM_code(uint8_t sensor_index, uint8_t **romCodePtr);

uint8_t ds18b20_read_ROM(uint8_t sensor_index);

void ds18b20_init(void);

uint8_t ds18b20_get_temperature_float(uint8_t sensor_index, float *temperature);

uint8_t ds18b20_read_resolution(uint8_t sensor_index);

uint8_t ds18b20_search_and_assign_ROM_codes(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* DS18B20_H_ */
