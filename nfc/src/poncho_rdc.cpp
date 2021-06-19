#include <stdbool.h>

#include "board.h"
#include "poncho_rdc.h"
#include "spi.h"

/**
 * @brief   initializes GPIOs for Poncho RDC card
 * @returns nothing
 * @note    outputs are set to high
 */
void poncho_rdc_init() {
    /* GPa 201204 Des-soldar puente SB6 para des-asociar a PHY RESET. Soldar PHY RESET a RESET general en mismo puente de la CIAA */
    Chip_SCU_PinMuxSet(6, 1, SCU_MODE_FUNC0);  // GPIO0    P6_1    PIN74    GPIO3[0]  RESET (SHARED)
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 3, 0);
    Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 3, 0);

    Chip_SCU_PinMuxSet(2, 5, (SCU_MODE_PULLUP | SCU_MODE_FUNC4));  // GPIO1 P2_5     PIN91    GPIO5[5]  SAMPLE (SHARED)
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 5);
    Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 5);

    Chip_SCU_PinMuxSet(7, 0, (SCU_MODE_PULLUP | SCU_MODE_FUNC0));  // GPIO2 P7_0     PIN110    GPIO3[8]  WR/FSYNC (ARM)
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 3, 8);
    Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 3, 8);

    Chip_SCU_PinMuxSet(6, 7, (SCU_MODE_PULLUP | SCU_MODE_FUNC4));  // GPIO3 P6_7    PIN85    GPIO5[15] WR/FSYNC (POLE)
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 15);
    Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);

    spi_init();
}

/**
 * @brief   handles the RESET line for the RDCs (shared)
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the chip resets with the LOW state of the RESET line
 */
void poncho_rdc_reset(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 3, 0);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 3, 0);
    }
}

/**
 * @brief   handles the SAMPLE line for the RDCs (shared)
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the chip copies the POS and VEL on the falling edge of the SAMPLE line
 */
void poncho_rdc_sample(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 5);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 5);
    }
}

/**
 * @brief   handles the WR/FSYNC line for the ARM RDC
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the falling edge of WR/FSYNC takes the SDI and SDO lines out of the high impedance state
 */
void poncho_rdc_arm_wr_fsync(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 3, 8);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 3, 8);
    }
}

/**
 * @brief   handles the WR/FSYNC line for the POLE RDC
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the falling edge of WR/FSYNC takes the SDI and SDO lines out of the high impedance state
 */
void poncho_rdc_pole_wr_fsync(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 15);
    }
}
