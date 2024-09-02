#pragma once

#include "board.h"

template<int scu_port, int scu_pin, int scu_mode, int gpio_port, int gpio_bit> class gpio_templ {
  public:
    static void init_output() {
        Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
        Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, gpio_port, gpio_bit);
    }

    static void init_input() {
        Chip_SCU_PinMuxSet(scu_port, scu_pin, scu_mode);
        Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, gpio_port, gpio_bit);
    }

    static void set() {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, true);
    }

    static void reset() {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, false);
    }

    static void set(bool state) {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit, state);
    }

    static void toggle() {
        Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, gpio_port, gpio_bit);
    }

    static bool read() {
        return Chip_GPIO_GetPinState(LPC_GPIO_PORT, gpio_port, gpio_bit);
    }
};

template<int scu_port, int scu_pin, int scu_mode, int gpio_port, int gpio_bit, LPC43XX_IRQn_Type IRQn>
class gpio_pinint_templ : public gpio_templ<scu_port, scu_pin, scu_mode, gpio_port, gpio_bit> {
  public:
    gpio_pinint_templ() {
        int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;

        /* Configure interrupt channel for the GPIO pin in SysCon block */
        Chip_SCU_GPIOIntPinSel(irq, gpio_port, gpio_bit);
        Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(irq));
    };

    gpio_pinint_templ &int_low() {
        int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;
        Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(irq));
        return *this;
    }

    gpio_pinint_templ &mode_edge() {
        int irq = static_cast<int>(IRQn) - PIN_INT0_IRQn;
        Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(irq));
        return *this;
    }

    gpio_pinint_templ &clear_pending() {
        NVIC_ClearPendingIRQ(IRQn);
        return *this;
    }

    gpio_pinint_templ &enable() {
        NVIC_EnableIRQ(IRQn);
        return *this;
    }

    gpio_pinint_templ &disable() {
        NVIC_DisableIRQ(IRQn);
        return *this;
    }

    gpio_pinint_templ &init_input() {
        gpio_templ<scu_port, scu_pin, scu_mode, gpio_port, gpio_bit>::init_input();
        return *this;
    }
};
