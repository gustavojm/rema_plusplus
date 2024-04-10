#include "board.h"
#include "eeprom.h"

/* Init EEPROM */
void EEPROM_init() {
    Chip_EEPROM_Init(LPC_EEPROM);

    /* Set Auto Programming mode */
    Chip_EEPROM_SetAutoProg(LPC_EEPROM, EEPROM_AUTOPROG_OFF);
}

/* Read data from EEPROM */
void EEPROM_Read(uint32_t pageOffset, uint32_t pageAddr, void *ptr,
        uint32_t size) {
    size = ((size + 3) & ~0x03) / 4;
    auto *pEepromMem = reinterpret_cast<uint32_t*>(EEPROM_ADDRESS(pageAddr,
            pageOffset));
    for (uint32_t i = 0; i < size; i++) {
        (reinterpret_cast<uint32_t*>(ptr))[i] = pEepromMem[i];
    }
}

/* Erase a page in EEPROM */
void EEPROM_Erase(uint32_t pageAddr) {
    auto *pEepromMem = reinterpret_cast<uint32_t*>(EEPROM_ADDRESS(pageAddr,
            0));
    for (int i = 0; i < EEPROM_PAGE_SIZE / 4; i++) {
        pEepromMem[i] = 0;
    }
    Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
}

/* Write data to a page in EEPROM */
void EEPROM_Write(uint32_t pageOffset, uint32_t pageAddr, void *ptr,
        uint32_t size) {
    size = ((size + 3) & ~0x03) / 4;
    auto *pEepromMem = reinterpret_cast<uint32_t*>(EEPROM_ADDRESS(pageAddr,
            pageOffset));

    if (size > EEPROM_PAGE_SIZE - pageOffset)
        size = EEPROM_PAGE_SIZE - pageOffset;

    for (uint32_t i = 0; i < size; i++) {
        pEepromMem[i] = (reinterpret_cast<uint32_t*>(ptr))[i];
    }

    Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
}
