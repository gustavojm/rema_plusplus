#include "encoders_pico.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "board.h"
#include "semphr.h"
#include "task.h"

#include "debug.h"
#include "mot_pap.h"
#include "quadrature_encoder_constants.h"
#include "rema.h"
#include "spi.h"

gpio_pinint encoders_irq_pin = {
    6, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0), 3, 0, PIN_INT0_IRQn
}; // GPIO0 P6_1     PIN74   GPIO3[0]


/**
 * @brief 	writes 1 byte (address or data) to the chip
 * @param 	data	: address or data to write through SPI
 * @returns	0 on success
 */
int32_t encoders_pico::write_register(uint8_t address, int32_t data) const {
    int32_t ret = 0;
    if (encoders_mutex != nullptr && xSemaphoreTake(encoders_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t write_address = address | quadrature_encoder_constants::WRITE_MASK;
        ret = spi_write(&write_address, 1, cs);

        uint8_t tx[4] = { static_cast<uint8_t>((data >> 24) & 0xFF),
                          static_cast<uint8_t>((data >> 16) & 0xFF),
                          static_cast<uint8_t>((data >> 8) & 0xFF),
                          static_cast<uint8_t>((data >> 0) & 0xFF) };
        ret = spi_write(&tx, 4, cs);
        xSemaphoreGive(encoders_mutex);
    }
    return ret;
}

/**
 * @brief 	reads value from one of the RASPBERRY PI PICO ENCODERS
 * @param 	address	: address to read through SPI
 * @returns	the value of the addressed record
 * @note	one extra register should be read because values come one
 * transfer after the address was put on the bus
 */
int32_t encoders_pico::read_register(uint8_t address) const {
    uint8_t rx[4] = { 0x00 };
    if (encoders_mutex != nullptr && xSemaphoreTake(encoders_mutex, portMAX_DELAY) == pdTRUE) {
        spi_write(&address, 1, cs);
        spi_read(rx, 4, cs);
        xSemaphoreGive(encoders_mutex);
    }
    return static_cast<int32_t>(rx[0] << 24 | rx[1] << 16 | rx[2] << 8 | rx[3] << 0);
}

/**
 * @brief 	reads value from the three of the RASPBERRY PI PICO ENCODERS
 * @param 	address	: address to read through SPI
 * @returns	the value of the addressed record
 * @note	one extra register should be read because values come one
 * transfer after the address was put on the bus
 */
void encoders_pico::read_4_registers(uint8_t address, uint8_t *rx) const {
    if (encoders_mutex != nullptr && xSemaphoreTake(encoders_mutex, portMAX_DELAY) == pdTRUE) {
        spi_write(&address, 1, cs);
        spi_read(rx, 4 * 4, cs);
        xSemaphoreGive(encoders_mutex);
    }
}

/**
 * @brief 	reads limits. (Used in telemetry)
 * @returns	status of the limits
 * @note
 */
struct limits encoders_pico::read_limits() const {
    uint8_t rx[4] = { 0x00 };
    if (encoders_mutex != nullptr && xSemaphoreTake(encoders_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t address = quadrature_encoder_constants::LIMITS;
        spi_write(&address, 1, cs);
        spi_read(rx, 4, cs);
        xSemaphoreGive(encoders_mutex);
    }
    return { rx[0], rx[1] };
}

/**
 * @brief 	reads limits. (Used in IRQ)
 * @returns	status of the limits
 * @note
 */
struct limits encoders_pico::read_limits_and_ack() const {
    uint8_t address = quadrature_encoder_constants::LIMITS | quadrature_encoder_constants::WRITE_MASK; // will ACK the IRQ
    uint8_t rx[4] = { 0x00 };

    if (encoders_mutex != nullptr && xSemaphoreTake(encoders_mutex, portMAX_DELAY) == pdTRUE) {
        spi_write(&address, 1, cs);
        spi_read(rx, 4, cs);
        xSemaphoreGive(encoders_mutex);
    }
    return { rx[0], rx[1] };
}

void encoders_pico::task([[maybe_unused]] void *pars) {
    encoders->set_thresholds(MOT_PAP_POS_THRESHOLD);

    NVIC_SetPriority(PIN_INT0_IRQn, ENCODERS_PICO_INTERRUPT_PRIORITY);
    encoders_irq_pin.init_input().mode_level().int_high();

    /* ATTENTION, THIS CODE SEEMS TO DO THE OPOSSITE BUT THERE MUST BE AN ERROR IN LPCOPEN  */
    encoders_irq_pin.int_low();
        
    encoders_irq_pin.enable();
    
    while (true) {
        if (xSemaphoreTake(encoders_pico_semaphore, portMAX_DELAY) == pdPASS) {
            struct limits limits = encoders->read_limits_and_ack();
            if (limits.hard & ENABLED_INPUTS_MASK) {
                rema::hard_limits_reached();
            }

            x_y_axes->first_axis->already_there = limits.targets & (1 << 0);
            x_y_axes->second_axis->already_there = limits.targets & (1 << 1);
            
            if (x_y_axes->first_axis->already_there && x_y_axes->second_axis->already_there) {
                x_y_axes->already_there = true;
                x_y_axes->stop();
                lDebug(Info, "%s: already there", x_y_axes->name);
            } else {
                x_y_axes->resume(); // Motors were paused by ISR to be able to read
                                    // encoders information
            }

            z_dummy_axes->first_axis->already_there = limits.targets & (1 << 2);
            if (z_dummy_axes->first_axis->already_there) {
                z_dummy_axes->already_there = true;
                z_dummy_axes->stop();
                lDebug(Info, "%s: already there", z_dummy_axes->name);
            } else {
                z_dummy_axes->resume(); // Motors were paused by ISR to be able to read
                                        // encoders information
            }
            //Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
            encoders_irq_pin.clear_pending().enable();
        }
    }
}

// IRQ Handler for Raspberry Pi Pico Encoders Reader...
extern "C" void GPIO0_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    x_y_axes->pause();
    z_dummy_axes->pause();
    xSemaphoreGiveFromISR(encoders_pico_semaphore, &xHigherPriorityTaskWoken);
    encoders_irq_pin.disable();                     // Otherwise IRQHandler will be called again immediately
                                                    // Reenabled at the end of encoders_pico::task
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void encoders_pico_init() {
    alignas(encoders_pico) static char encoders_pico_buf[sizeof(encoders_pico)];
    encoders = new (encoders_pico_buf) encoders_pico();
}
