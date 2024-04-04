#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <cstdint>

#include "bresenham.h"
#include "encoders_pico.h"


class tcp_server {
public:
    explicit tcp_server(const char *name, int port, bresenham &x_y, bresenham &z_dummy, encoders_pico &encoders, void (*reply_fn)(const int sock));

    void task();  
    
    const char *name;
    int port;
    bresenham &x_y;
    bresenham &z_dummy;
    encoders_pico &encoders;

    void (*reply_fn)(const int sock);
};


#endif /* TCP_SERVER_H_ */
