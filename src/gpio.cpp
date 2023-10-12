#include "board.h"
#include "gpio.h"
#include "mot_pap.h"

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
gpio& gpio::init_output() {
	Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, gpio_port, gpio_bit);
	return *this;
}

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
gpio& gpio::init_input() {
	Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, gpio_port, gpio_bit);
	return *this;
}

/**
 * @brief    GPIO sets pin passed to high
 * @returns nothing
 */
gpio& gpio::set() {
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, true);
    return *this;
}

/**
 * @brief    GPIO sets pin passed to low
 * @returns nothing
 */
gpio& gpio::reset() {
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, false);
    return *this;
}

/**
 * @brief	 GPIO sets pin passed to the state specified
 * @returns nothing
 */
gpio& gpio::set(bool state) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, state);
	return *this;
}

/**
 * @brief    GPIO sets pin passed as parameter to the state specified
 * @returns nothing
 */
bool gpio::read() {
    return Chip_GPIO_GetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit);
}

/**
 * @brief	toggles GPIO corresponding pin passed as parameter
 * @returns nothing
 */
gpio&  gpio::toggle() {
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, gpio_port, gpio_bit);
	return *this;
}
