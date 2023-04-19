#include <stdint.h>
#include "probe.h"
#include "board.h"
#include "mot_pap.h"
#include "encoders.h"
#include "gpio.h"
#include "rema.h"

/**
 * @brief   Main program body
 * @return  Does not return
 */
probe::probe(gpio_pinint gpio) : gpio(gpio) {
            gpio.init_input()
            .mode_edge()
            .int_low()
            .clear_pending()
            .enable();
}

void probe::enable() {
    gpio
    .clear_pending()
    .enable();
}

void probe::disable() {
    gpio.disable();
}
