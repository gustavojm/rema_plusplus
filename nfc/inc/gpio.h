#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_entry {
	int pin_port;
	int pin_bit;
	int scu_mode;
	int gpio_port;
	int gpio_bit;
};

void gpio_init_output(struct gpio_entry gpio);

void gpio_init_input(struct gpio_entry gpio);

void gpio_toggle(struct gpio_entry gpio);

void gpio_set_pin_state(struct gpio_entry gpio, bool state);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
