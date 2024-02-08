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

#define ENCODERS_PICO_READ_COUNTERS      0x60
#define ENCODERS_PICO_READ_COUNTER_X     ENCODERS_PICO_READ_COUNTERS + 1
#define ENCODERS_PICO_READ_COUNTER_Y     ENCODERS_PICO_READ_COUNTERS + 2
#define ENCODERS_PICO_READ_COUNTER_Z     ENCODERS_PICO_READ_COUNTERS + 3
#define ENCODERS_PICO_READ_COUNTER_W     ENCODERS_PICO_READ_COUNTERS + 4


#define ENCODERS_PICO_LOAD_COUNTERS      0xE0
#define ENCODERS_PICO_LOAD_COUNTER_X     ENCODERS_PICO_LOAD_COUNTERS + 1
#define ENCODERS_PICO_LOAD_COUNTER_Y     ENCODERS_PICO_LOAD_COUNTERS + 2
#define ENCODERS_PICO_LOAD_COUNTER_Z     ENCODERS_PICO_LOAD_COUNTERS + 3
#define ENCODERS_PICO_LOAD_COUNTER_W     ENCODERS_PICO_LOAD_COUNTERS + 4

#define ENCODERS_PICO_SOFT_RESET         0XFF


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
 * @brief    RDC device instance specific state.
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

    int32_t read_register(uint8_t address) const;

    int32_t write_register(uint8_t address, int32_t data) const;

    int32_t soft_reset() const;

private:
    void (*cs)(bool) = cs_function;             ///< pointer to CS line function handler
};

#endif /* ENCODERS_PICO_H_ */
