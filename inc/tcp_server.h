#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <cstdint>

#include "bresenham.h"
#include "encoders_pico.h"


class tcp_server {
public:
    explicit tcp_server(const char *name, int port, bresenham &x_y, bresenham &z_dummy, encoders_pico &encoders);

    virtual ~tcp_server() {} // Virtual destructor

    virtual void reply_fn(int sock) = 0;

    void task();  
    
    const char *name;
    int port;
    bresenham &x_y;
    bresenham &z_dummy;
    encoders_pico &encoders;
};


#endif /* TCP_SERVER_H_ */
