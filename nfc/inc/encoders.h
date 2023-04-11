#ifndef ENCODERS_H_
#define ENCODERS_H_

#ifdef __cplusplus
extern "C" {
#endif

void encoders_init();

void GPIO5_IRQHandler(void);
void GPIO6_IRQHandler(void);
void GPIO7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODERS_H_ */


