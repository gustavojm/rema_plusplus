#ifndef ENCODERS_H_
#define ENCODERS_H_

void encoders_init();


#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief    Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return   Nothing
*/
void GPIO5_IRQHandler(void);
void GPIO6_IRQHandler(void);
void GPIO7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODERS_H_ */


