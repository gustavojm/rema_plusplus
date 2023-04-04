#include "board.h"
#include "gpio.h"
#include "mot_pap.h"

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio::init_output() {
	Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, gpio_port, gpio_bit);
}

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio::init_input() {
	Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, gpio_port, gpio_bit);
}

/**
 * @brief	 GPIO sets pin passed as parameter to the state specified
 * @returns nothing
 */
void gpio::set_pin_state(bool state) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, state);
}

/**
 * @brief	toggles GPIO corresponding pin passed as parameter
 * @returns nothing
 */
void gpio::toggle() {
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, gpio_port, gpio_bit);
}
