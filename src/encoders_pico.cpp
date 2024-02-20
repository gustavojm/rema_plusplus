#include "encoders_pico.h"

#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "spi.h"
#include "quadrature_encoder_constants.h"

gpio_templ< 4, 4, SCU_MODE_FUNC0, 2, 4 > click;      // DOUT3 P4_4    PIN9    GPIO2[4]>
SemaphoreHandle_t encoders_pico_semaphore;

/**
 * @brief 	writes 1 byte (address or data) to the chip
 * @param 	data	: address or data to write through SPI
 * @returns	0 on success
 */
int32_t encoders_pico::write_register(uint8_t address, int32_t data) const {
    int32_t ret = 0;
    uint8_t write_address = address | quadrature_encoder_constants::WRITE_MASK;
    ret = spi_write(&write_address, 1, cs);

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
 * @brief 	reads value from the three of the RASPBERRY PI PICO ENCODERS
 * @param 	address	: address to read through SPI
 * @returns	the value of the addressed record
 * @note	one extra register should be read because values come one transfer after the
 * 			address was put on the bus
 */
void encoders_pico::read_4_registers(uint8_t address, uint8_t *rx) const {
    spi_write(&address, 1, cs);
    spi_read(rx, 4 * 4, cs);
}

/**
 * @brief 	reads limits.
 * @returns	status of the limits
 * @note	
 */
struct limits encoders_pico::read_limits() const {
    uint8_t address = quadrature_encoder_constants::LIMITS | quadrature_encoder_constants::WRITE_MASK;         // will ACK the IRQ
    spi_write(&address, 1, cs);

    uint8_t rx[4] = {0x00};
    spi_read(rx, 4, cs);     
    return {rx[0], rx[1]};
}

// IRQ Handler for Raspberry Pi Pico Encoders Reader...
extern "C" void GPIO0_IRQHandler(void) {
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
    static bool status = false;
    status = !status;
    click.init_output();
    click.set(status);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(encoders_pico_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}