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
            char *dbg_msg;            
            if (xQueueReceive(debug_queue, &dbg_msg, portMAX_DELAY) == pdPASS) {
                size_t msg_len = strlen(dbg_msg);

                if (msg_len > 0) {
                    msg_len ++;         // as strlen do not take the null terminator into account
                    lDebug(InfoLocal, "To send %d bytes: %s", msg_len, dbg_msg);
                    
                    // send() can return less bytes than supplied length.
                    // Walk-around for robust implementation.
                    int to_write = msg_len;
                    while (to_write > 0) {
                        int written = lwip_send(sock, dbg_msg + (msg_len - to_write), to_write, 0);
                        if (written < 0) {
                            //lDebug(Error, "Error occurred during sending logs: errno %d", errno);
                            goto cleanup;
                        }
                        to_write -= written;
                    }
                } else {
                    //lDebug(Error, "buffer too small");
                }
cleanup:
                delete[] dbg_msg;
                dbg_msg = nullptr;
            }
        }
    }
};
