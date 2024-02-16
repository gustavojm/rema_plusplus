#ifndef ENCODERS_PICO_H_
#define ENCODERS_PICO_H_

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "spi.h"

/* Control register bits description */
#define ENCODERS_PICO_CLEAR_COUNTERS     0x20
#define ENCODERS_PICO_CLEAR_COUNTER_X    ENCODERS_PICO_CLEAR_COUNTERS + 1
#define ENCODERS_PICO_CLEAR_COUNTER_Y    ENCODERS_PICO_CLEAR_COUNTERS + 2
#define ENCODERS_PICO_CLEAR_COUNTER_Z    ENCODERS_PICO_CLEAR_COUNTERS + 3
#define ENCODERS_PICO_CLEAR_COUNTER_W    ENCODERS_PICO_CLEAR_COUNTERS + 4

#define ENCODERS_PICO_COUNTERS          0x30
#define ENCODERS_PICO_COUNTER_X         ENCODERS_PICO_COUNTERS + 1
#define ENCODERS_PICO_COUNTER_Y         ENCODERS_PICO_COUNTERS + 2
#define ENCODERS_PICO_COUNTER_Z         ENCODERS_PICO_COUNTERS + 3
#define ENCODERS_PICO_COUNTER_W         ENCODERS_PICO_COUNTERS + 4

#define ENCODERS_PICO_TARGETS           0x40
#define ENCODERS_PICO_TARGET_X          ENCODERS_PICO_TARGETS + 1
#define ENCODERS_PICO_TARGET_Y          ENCODERS_PICO_TARGETS + 2
#define ENCODERS_PICO_TARGET_Z          ENCODERS_PICO_TARGETS + 3
#define ENCODERS_PICO_TARGET_W          ENCODERS_PICO_TARGETS + 4

#define ENCODERS_PICO_SOFT_RESET    0XFF


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
    }

    ~encoders_pico() {
        spi_de_init();
    }

    static encoders_pico& get_instance() {
        static encoders_pico instance;
        return instance;
    }

    int32_t read_register(uint8_t address) const;

    void read_3_registers(uint8_t address, uint8_t *rx) const;

    int32_t write_register(uint8_t address, int32_t data) const;

    int32_t soft_reset() const;

    uint8_t read_hard_limits() const;

    void ack_hard_limits() const;

public:
    void (*cs)(bool) = cs_function;             ///< pointer to CS line function handler
};

#endif /* ENCODERS_PICO_H_ */
