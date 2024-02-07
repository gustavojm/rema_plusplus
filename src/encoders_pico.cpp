#include "encoders_pico.h"

#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "spi.h"

/**
 * @brief 	writes 1 byte (address or data) to the chip
 * @param 	data	: address or data to write through SPI
 * @returns	0 on success
 */
int32_t encoders_pico::write_register(uint8_t address, int32_t data) const {
    int32_t ret = 0;

    ret = spi_write(&address, 1, cs);

    uint8_t tx[5] = {static_cast<uint8_t>((data >> 24) & 0xFF), 
                     static_cast<uint8_t>((data >> 16) & 0xFF), 
                     static_cast<uint8_t>((data >> 8) & 0xFF), 
                     static_cast<uint8_t>((data >> 0) & 0xFF)
                    };
    ret = spi_write(&tx, 4, cs);

    return ret;
}


/**
 * @brief 	reads value from one of the RASPBERRY PI PICO ENCODERS
 * @param 	address	: address to read through SPI
 * @returns	the value of the addressed record
 * @note	one extra register should be read because values come one transfer after the
 * 			address was put on the bus
 */
int32_t encoders_pico::read_register(uint8_t address) const {    
    spi_write(&address, 1, cs);

    uint8_t rx[4] = {0x00};
    spi_read(rx, 4, cs);
    return static_cast<int32_t>(rx[0] << 24 | rx[1] << 16 | rx[2] << 8 | rx[3] << 0);
}

/**
 * @brief 	soft resets the chip.
 * @returns	0 on success
 * @note	
 */
int32_t encoders_pico::soft_reset() const {
    int32_t ret;
    uint8_t tx = ENCODERS_PICO_SOFT_RESET;
    ret = spi_write(&tx, 1, cs);
    return ret;
}
