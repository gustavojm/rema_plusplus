#ifndef POLE_H_
#define POLE_H_

#include <stdint.h>
#include <stdbool.h>

#include "mot_pap.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declaration needed because TEST_GUI calls this IRQ handler as a standard function
void TIMER0_IRQHandler(void);

void pole_init();

struct mot_pap *pole_get_status(void);

uint16_t pole_get_RDC_position();

void pole_set_offset(uint16_t offset);

JSON_Value *pole_json(void);

#ifdef __cplusplus
}
#endif

#endif /* POLE_H_ */
