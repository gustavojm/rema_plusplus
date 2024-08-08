#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "debug.h"

#include "arduinojson_cust_alloc.h"
#include "encoders_pico.h"
#include "rema.h"
#include "tcp_server.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"

namespace json = ArduinoJson;

class tcp_server_logs : public tcp_server {
  public:
    tcp_server_logs(int port) : tcp_server("logs", port) {
    }

    void reply_fn(int sock) override {
        while (true) {
            char *debug_msg;            
            if (xQueueReceive(debug_queue, &debug_msg, portMAX_DELAY) == pdPASS) {
                size_t msg_len = strlen(debug_msg);

                //tx_buffer[msg_len] = '\0'; // null terminate

                if (msg_len > 0) {                    
                    msg_len++;
                    //lDebug(InfoLocal, "To send %d bytes: %s", msg_len, tx_buffer);
                    
                    // send() can return less bytes than supplied length.
                    // Walk-around for robust implementation.
                    int to_write = msg_len;
                    while (to_write > 0) {
                        int written = lwip_send(sock, debug_msg + (msg_len - to_write), to_write, 0);
                        delete[] debug_msg;
                        debug_msg = nullptr;

                        if (written < 0) {
                            //lDebug(Error, "Error occurred during sending logs: errno %d", errno);
                            return;
                        }
                        to_write -= written;
                    }
                } else {
                    //lDebug(Error, "buffer too small");
                }
            }
        }
    }
};
