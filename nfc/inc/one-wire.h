#ifndef ONE_WIRE_H
#define ONE_WIRE_H

#include "one-wire_config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* error codes */
#define ERR_OK                          0x00U /*!< OK */
#define ERR_BUSOFF                      0x0CU /*!< Bus not available. */
#define ERR_CRC                         0x14U /*!< CRC error is detected. */
#define ERR_RANGE                       0x02U /*!< Parameter out of range. */

#define ONE_WIRE_ROM_CODE_SIZE   (8)
/*!< Number of bytes for ROM code */

uint8_t one_wire_count(void);

uint8_t one_wire_receive(uint8_t counter);

uint8_t one_wire_send_byte(uint8_t data);

uint8_t one_wire_send_bytes(uint8_t *data, uint8_t count);

uint8_t one_wire_send_reset(void);

uint8_t one_wire_get_byte(uint8_t *data);

uint8_t one_wire_get_bytes(uint8_t *data, uint8_t count);

void one_wire_init(void);

void one_wire_deinit(void);

uint8_t one_wire_calc_CRC(uint8_t *data, uint8_t dataSize);

uint8_t one_wire_read_rom_code(uint8_t *romCodeBuffer);

uint8_t one_wire_strcatRomCode(uint8_t *buf, size_t bufSize, uint8_t *romCode);

void one_wire_reset_search(void);

void one_wire_target_search(uint8_t familyCode);

bool one_wire_search(uint8_t *newAddr, bool search_mode);

void one_wire_strong_pull_up(uint32_t ms);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* ONE_WIRE_H_ */
