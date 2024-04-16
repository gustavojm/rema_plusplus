#ifndef ENCODERS_PICO_H_
#define ENCODERS_PICO_H_

#include <stdint.h>
#include <stdbool.h>
#include <new>

#include "FreeRTOS.h"
#include "semphr.h"

#include "board.h"
#include "spi.h"
#include "gpio_templ.h"
#include "gpio.h"
#include "debug.h"
#include "quadrature_encoder_constants.h"

#define ENCODERS_PICO_TASK_PRIORITY       ( configMAX_PRIORITIES - 1)
#define ENCODERS_PICO_INTERRUPT_PRIORITY  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)   //Has to have higher priority than timers ( now +2 )

extern SemaphoreHandle_t encoders_pico_semaphore;

struct limits {
    uint8_t hard;
    uint8_t targets;
};

/**
 * @brief   handles the CS line for the ENCODERS RASPBERRY PI PICO
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the falling edge of CS takes the SDI and SDO lines out of the high impedance state???
 */
static void cs_function(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 15);
    }
}

/**
 * @struct     encoders_pico
 * @brief    Raspberry-pi Pico device instance specific state.
 */
class encoders_pico {
public:
    encoders_pico() {
        Chip_SCU_PinMuxSet(6, 7, (SCU_MODE_PULLUP | SCU_MODE_FUNC4));  // GPIO3 P6_7    PIN85    GPIO5[15] CS ENCODER RASPBERRY PI PICO
        Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 15);
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);
        spi_init();

        encoders_pico_semaphore = xSemaphoreCreateBinary();

        if (encoders_pico_semaphore != NULL) {
            // Create the 'handler' task, which is the task to which interrupt processing is deferred
            xTaskCreate(encoders_pico::task, "encoders_pico",
            256,
            NULL, ENCODERS_PICO_TASK_PRIORITY, NULL);
            lDebug(Info, "encoders_pico_task created");
        }

    }

    ~encoders_pico() {
        spi_de_init();
    }

    static void task(void *pars);

    int32_t read_register(uint8_t address) const;

    void read_4_registers(uint8_t address, uint8_t *rx) const;

    int32_t write_register(uint8_t address, int32_t data) const;

    struct limits read_limits() const;

    struct limits read_limits_and_ack() const;

    void set_target(char axis, int target) {
        write_register(quadrature_encoder_constants::TARGETS + (axis - 'X') + 1, target);
    }

    void set_counter(char axis, int32_t data) {
        write_register(quadrature_encoder_constants::COUNTERS + (axis - 'X') + 1, data);
    }

    int read_counter(char axis) {
        return read_register(quadrature_encoder_constants::COUNTERS + (axis - 'X') + 1);
    }

    void read_counters(uint8_t *rx) {
        read_4_registers(quadrature_encoder_constants::COUNTERS, rx);
    }

    void set_thresholds(int threshold) {
        write_register(quadrature_encoder_constants::POS_THRESHOLDS, threshold);
    }

    void set_direction(char axis, bool dir) {
        write_register(quadrature_encoder_constants::DIRECTIONS + (axis - 'X') + 1, dir);
    }


public:
    void (*cs)(bool) = cs_function;             ///< pointer to CS line function handler
};

void encoders_pico_init();

#endif /* ENCODERS_PICO_H_ */

