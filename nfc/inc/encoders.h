#ifndef ENCODERS_H_
#define ENCODERS_H_

#ifdef __cplusplus
extern "C" {
#endif

void encoders_init();

void GPIO0_IRQHandler(void);
void GPIO1_IRQHandler(void);
void GPIO2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODERS_H_ */


