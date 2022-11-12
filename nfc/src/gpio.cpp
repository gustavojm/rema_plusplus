#include <stdbool.h>

#include "board.h"
#include "gpio.h"
#include "mot_pap.h"

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio_init_output(struct gpio_entry gpio) {
	Chip_SCU_PinMuxSet(gpio.pin_port, gpio.pin_bit, gpio.scu_mode);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio_init_input(struct gpio_entry gpio) {
	Chip_SCU_PinMuxSet(gpio.pin_port, gpio.pin_bit, gpio.scu_mode);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}


/**
 * @brief	 GPIO sets pin passed as parameter to the state specified
 * @returns nothing
 */
void gpio_set_pin_state(struct gpio_entry gpio, bool state) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit, state);
}

/**
 * @brief	toggles GPIO corresponding pin passed as parameter
 * @returns nothing
 */
void gpio_toggle(struct gpio_entry gpio) {
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}
