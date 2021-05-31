#ifndef ARM_H_
#define ARM_H_

#include <stdint.h>
#include <stdbool.h>

#include "mot_pap.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declaration needed because TEST_GUI calls this IRQ handler as a standard function
void TIMER1_IRQHandler(void);

void arm_init();

struct mot_pap *arm_get_status(void);

uint16_t arm_get_RDC_position();

void arm_set_offset(uint16_t offset);

JSON_Value *arm_json(void);

#ifdef __cplusplus
}
#endif

#endif /* ARM_H_ */
