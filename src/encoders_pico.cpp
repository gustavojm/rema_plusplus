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
#include "mot_pap.h"
#include "rema.h"

SemaphoreHandle_t encoders_pico_semaphore;
extern mot_pap x_axis, y_axis, z_axis;

/**
 * @brief 	writes 1 byte (address or data) to the chip
 * @param 	data	: address or data to write through SPI
 * @returns	0 on success
 */
int32_t encoders_pico::write_register(uint8_t address, int32_t data) const {
    int32_t ret = 0;
    uint8_t write_address = address | quadrature_encoder_constants::WRITE_MASK;
    ret = spi_write(&write_address, 1, cs);

    uint8_t tx[4] = {static_cast<uint8_t>((data >> 24) & 0xFF), 
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
 * @brief 	reads limits. (Used in telemetry)
 * @returns	status of the limits
 * @note	
 */
struct limits encoders_pico::read_limits() const {
    uint8_t address = quadrature_encoder_constants::LIMITS;
    spi_write(&address, 1, cs);

    uint8_t rx[4] = {0x00};
    spi_read(rx, 4, cs);     
    return {rx[0], rx[1]};
}

/**
 * @brief 	reads limits. (Used in IRQ)
 * @returns	status of the limits
 * @note	
 */
struct limits encoders_pico::read_limits_and_ack() const {
    uint8_t address = quadrature_encoder_constants::LIMITS | quadrature_encoder_constants::WRITE_MASK;         // will ACK the IRQ
    spi_write(&address, 1, cs);

    uint8_t rx[4] = {0x00};
    spi_read(rx, 4, cs);     
    return {rx[0], rx[1]};
}


void encoders_pico::task(void *pars) {
    gpio_pinint encoders_irq_pin = {6, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0), 3, 0, PIN_INT0_IRQn};   //GPIO5 P6_1     PIN74   GPIO3[0]    
    encoders_irq_pin.init_input()
            .mode_edge()
            .int_high()
            .clear_pending();

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));

    NVIC_SetPriority(PIN_INT0_IRQn, ENCODERS_PICO_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(PIN_INT0_IRQn);

    encoders_pico &encoders = encoders_pico::get_instance();
    encoders.set_thresholds(MOT_PAP_POS_THRESHOLD);

    while (true) {
        if (xSemaphoreTake(encoders_pico_semaphore, portMAX_DELAY) == pdPASS) {
            auto &encoders = encoders_pico::get_instance();
            struct limits limits = encoders.read_limits_and_ack();
            if (limits.hard) {
                rema::hard_limits_reached();
            }

            x_axis.already_there = limits.targets & 1 << 0;
            y_axis.already_there = limits.targets & 1 << 1;
            z_axis.already_there = limits.targets & 1 << 2;

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// IRQ Handler for Raspberry Pi Pico Encoders Reader...
extern "C" void GPIO0_IRQHandler(void) {
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(encoders_pico_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    while(true);
}
