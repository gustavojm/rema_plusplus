#pragma once

#include <cstdint>

#include "bresenham.h"
#include "encoders_pico.h"

class tcp_server {
  public:
    explicit tcp_server(const char *name, int port);

    virtual ~tcp_server() {
    } // Virtual destructor

    virtual void reply_fn(int sock) = 0;

    void task();

    const char *name;
    int port;
};
