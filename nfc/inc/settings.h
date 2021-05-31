#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdbool.h>
#include "eeprom.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct settings {
	ip_addr_t	gw;
	ip_addr_t	ipaddr;
	ip_addr_t	netmask;
	uint16_t	port;
};

void settings_init();

void settings_erase(void);

void settings_save(struct settings settings);

struct settings settings_read();

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
