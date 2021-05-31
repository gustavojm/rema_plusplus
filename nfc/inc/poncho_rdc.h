#ifndef PONCHO_RDC_H_
#define PONCHO_RDC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void poncho_rdc_init();

void poncho_rdc_reset(bool state);

void poncho_rdc_sample(bool state);

void poncho_rdc_arm_wr_fsync(bool state);

void poncho_rdc_pole_wr_fsync(bool state);

#ifdef __cplusplus
}
#endif

#endif /* PONCHO_RDC_H_ */
