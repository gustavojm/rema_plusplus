#ifndef PROBE_H_
#define PROBE_H_

#include "gpio.h"

class probe {
public:

    probe(gpio_pinint gpio);

    void enable();

    void disable();

    gpio_pinint gpio;
};

#endif /* PROBE_H_ */


