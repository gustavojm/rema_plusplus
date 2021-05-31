#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Init EEPROM */
void EEPROM_init(void);

/* Read data from EEPROM */
/* size must be multiple of 4 bytes */
void EEPROM_Read(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size);

/* Erase a page in EEPROM */
void EEPROM_Erase(uint32_t pageAddr);

/* Write data to a page in EEPROM */
/* size must be multiple of 4 bytes */
void EEPROM_Write(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size);


#ifdef __cplusplus
}
#endif

#endif /* EEPROM_H_ */
