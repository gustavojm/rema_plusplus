#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#ifdef __cplusplus
extern "C" {
#endif

void temperature_init();

uint16_t temperature_read(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_H_ */
