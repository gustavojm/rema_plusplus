#pragma once

#include "board.h"

class gpio {
public:
  gpio() = default;

  gpio(int scu_port, int scu_pin, int scu_mode, int gpio_port, int gpio_bit)
      : scu_port(scu_port), scu_pin(scu_pin), scu_mode(scu_mode),
        gpio_port(gpio_port), gpio_bit(gpio_bit) {}

  gpio &init_output();

  gpio &init_input();

  gpio &toggle();

  gpio &set();

  gpio &reset();

  gpio &set(bool state);

  bool read() const;

  int scu_port;
  int scu_pin;
  int scu_mode;
  int gpio_port;
  int gpio_bit;
};

class gpio_pinint : public gpio {
public:
  LPC43XX_IRQn_Type IRQn;

  gpio_pinint(int scu_port, int scu_pin, int scu_mode, int gpio_port,
              int gpio_bit, LPC43XX_IRQn_Type IRQn)
      : gpio(scu_port, scu_pin, scu_mode, gpio_port, gpio_bit), IRQn(IRQn) {
    int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;

    /* Configure interrupt channel for the GPIO pin in SysCon block */
    Chip_SCU_GPIOIntPinSel(irq, gpio_port, gpio_bit);
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(irq));
  };

  gpio_pinint &int_low() {
    int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;
    Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(irq));
    return *this;
  }

  gpio_pinint &int_high() {
    int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;
    Chip_PININT_EnableIntHigh(LPC_GPIO_PIN_INT, PININTCH(irq));
    return *this;
  }

  gpio_pinint &mode_edge() {
    int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;
    Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(irq));
    return *this;
  }

  gpio_pinint &clear_pending() {
    NVIC_ClearPendingIRQ(IRQn);
    return *this;
  }

  gpio_pinint &enable() {
    NVIC_EnableIRQ(IRQn);
    return *this;
  }

  gpio_pinint &disable() {
    NVIC_DisableIRQ(IRQn);
    return *this;
  }

  gpio_pinint &init_input() {
    gpio::init_input();
    return *this;
  }
};
