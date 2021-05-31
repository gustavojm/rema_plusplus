#include "board.h"
#include "eeprom.h"

/* Init EEPROM */
void EEPROM_init(void)
{
	Chip_EEPROM_Init(LPC_EEPROM);

	/* Set Auto Programming mode */
	Chip_EEPROM_SetAutoProg(LPC_EEPROM, EEPROM_AUTOPROG_OFF);
}

/* Read data from EEPROM */
void EEPROM_Read(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size)
{
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, pageOffset);
	for (int i = 0; i < size / 4; i++) {
		ptr[i] = pEepromMem[i];
	}
}

/* Erase a page in EEPROM */
void EEPROM_Erase(uint32_t pageAddr)
{
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, 0);
	for (int i = 0; i < EEPROM_PAGE_SIZE / 4; i++) {
		pEepromMem[i] = 0;
	}
	Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
}

/* Write data to a page in EEPROM */
void EEPROM_Write(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size)
{
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, pageOffset);

	if (size > EEPROM_PAGE_SIZE - pageOffset)
		size = EEPROM_PAGE_SIZE - pageOffset;

	for (int i = 0; i < size / 4; i++) {
		pEepromMem[i] = ptr[i];
	}

	Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
}
