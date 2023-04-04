#ifndef RELAY_H_
#define RELAY_H_

#ifdef __cplusplus
extern "C" {
#endif

void relay_init();

void relay_main_pwr(bool state);

void relay_spare_led(bool state);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_H_ */
