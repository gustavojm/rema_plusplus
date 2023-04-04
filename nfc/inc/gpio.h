#ifndef GPIO_H_
#define GPIO_H_

class gpio {
public:
    gpio() = default;

    gpio(int scu_port, int scu_pin, int scu_mode, int gpio_port, int gpio_bit)
        : scu_port(scu_port), scu_pin(scu_pin), scu_mode(scu_mode), gpio_port(gpio_port), gpio_bit(gpio_bit)
    {}

    void init_output();

    void init_input();

    void toggle();

    void set_pin_state(bool state);

	int scu_port;
	int scu_pin;
	int scu_mode;
	int gpio_port;
	int gpio_bit;
};

#endif /* GPIO_H_ */
