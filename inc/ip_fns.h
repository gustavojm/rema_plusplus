#pragma once

#include <cstdio>
#include <cstdint>

#include "../lwip/inc/ipv4/lwip/ip_addr.h"

void ipaddr_to_dot_format(ip_addr_t ip_address, char * out) {     
    sprintf(out, "%d.%d.%d.%d", ip4_addr1(&ip_address),
                                ip4_addr2(&ip_address),
                                ip4_addr3(&ip_address),
                                ip4_addr4(&ip_address));                            
}