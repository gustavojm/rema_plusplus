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

class tcp_server_telemetry : public tcp_server {
  public:
    tcp_server_telemetry(int port) : tcp_server("telemetry", port) {
    }

    void reply_fn(int sock) override {
        const int buf_len = 1024;
        char tx_buffer[buf_len];
        json::MyJsonDocument ans;

        int times = 0;

        while (true) {
            x_y_axes->first_axis->read_pos_from_encoder();
            x_y_axes->second_axis->read_pos_from_encoder();
            z_dummy_axes->first_axis->read_pos_from_encoder();

            ans["telemetry"]["coords"]["x"] =
                x_y_axes->first_axis->current_counts / static_cast<double>(x_y_axes->first_axis->inches_to_counts_factor);
            ans["telemetry"]["coords"]["y"] =
                x_y_axes->second_axis->current_counts / static_cast<double>(x_y_axes->second_axis->inches_to_counts_factor);
            ans["telemetry"]["coords"]["z"] = z_dummy_axes->first_axis->current_counts /
                                              static_cast<double>(z_dummy_axes->first_axis->inches_to_counts_factor);

            ans["telemetry"]["targets"]["x"] = x_y_axes->first_axis->destination_counts /
                                               static_cast<double>(x_y_axes->first_axis->inches_to_counts_factor);
            ans["telemetry"]["targets"]["y"] = x_y_axes->second_axis->destination_counts /
                                               static_cast<double>(x_y_axes->second_axis->inches_to_counts_factor);
            ans["telemetry"]["targets"]["z"] = z_dummy_axes->first_axis->destination_counts /
                                               static_cast<double>(z_dummy_axes->first_axis->inches_to_counts_factor);

            struct limits limits = encoders->read_limits();

            ans["telemetry"]["limits"]["left"] = static_cast<bool>(limits.hard & 1 << 0);
            ans["telemetry"]["limits"]["right"] = static_cast<bool>(limits.hard & 1 << 1);
            ans["telemetry"]["limits"]["up"] = static_cast<bool>(limits.hard & 1 << 2);
            ans["telemetry"]["limits"]["down"] = static_cast<bool>(limits.hard & 1 << 3);
            ans["telemetry"]["limits"]["in"] = static_cast<bool>(limits.hard & 1 << 4);
            ans["telemetry"]["limits"]["out"] = static_cast<bool>(limits.hard & 1 << 5);
            ans["telemetry"]["limits"]["probe"] = touch_probe_irq_pin.read();

            ans["telemetry"]["control_enabled"] = rema::control_enabled;
            ans["telemetry"]["stall_control"] = rema::stall_control;
            ans["telemetry"]["brakes_mode"] = static_cast<int>(rema::brakes_mode);

            ans["telemetry"]["stalled"]["x"] = x_y_axes->first_axis->stalled;
            ans["telemetry"]["stalled"]["y"] = x_y_axes->second_axis->stalled;
            ans["telemetry"]["stalled"]["z"] = z_dummy_axes->first_axis->stalled;

            ans["telemetry"]["probe"]["x_y"] = x_y_axes->was_stopped_by_probe;
            ans["telemetry"]["probe"]["z"] = z_dummy_axes->was_stopped_by_probe;
            ans["telemetry"]["probe_protected"] =
                x_y_axes->was_stopped_by_probe_protection || z_dummy_axes->was_stopped_by_probe_protection;

            ans["telemetry"]["on_condition"]["x_y"] =
                (x_y_axes->already_there && !x_y_axes->was_soft_stopped); // Soft stops are only sent by joystick,
                                                                          // so no ON_CONDITION reported
            ans["telemetry"]["on_condition"]["z"] = (z_dummy_axes->already_there && !z_dummy_axes->was_soft_stopped);

            if (!(times % 50)) {
                ans["temps"]["x"] = (static_cast<double>(temperature_ds18b20_get(0))) / 10;
                ans["temps"]["y"] = (static_cast<double>(temperature_ds18b20_get(1))) / 10;
                ans["temps"]["z"] = (static_cast<double>(temperature_ds18b20_get(2))) / 10;
            }
            times++;

            size_t msg_len = json::serializeMsgPack(ans, tx_buffer, sizeof(tx_buffer) - 1);

            tx_buffer[msg_len] = '\0'; // null terminate
            msg_len++;

            //
            //lDebug(InfoLocal, "To send %d bytes: %s", msg_len, tx_buffer);

            if (msg_len > 0) {
                // send() can return less bytes than supplied length.
                // Walk-around for robust implementation.
                int to_write = msg_len;
                while (to_write > 0) {
                    int written = lwip_send(sock, tx_buffer + (msg_len - to_write), to_write, 0);
                    if (written < 0) {
                        //lDebug(Error, "Error occurred during sending telemetry: errno %d", errno);
                        return;
                    }
                    to_write -= written;
                }
            } else {
                //lDebug(Error, "buffer too small");
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
};
