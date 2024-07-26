#ifndef TCP_SERVER_TELEMETRY_H_
#define TCP_SERVER_TELEMETRY_H_

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "debug.h"

#include "arduinojson_cust_alloc.h"
#include "rema.h"
#include "tcp_server.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "telemetry.pb.h"

extern bresenham *x_y_axes, *z_dummy_axes;
extern encoders_pico *encoders;

// Function to serialize telemetry data
bool serialize_telemetry(const Telemetry *telemetry, uint8_t *buffer, size_t buffer_size, size_t *message_length) {
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    if (!pb_encode(&stream, Telemetry_fields, telemetry)) {
        fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *message_length = stream.bytes_written;
    return true;
}

// Function to deserialize telemetry data
bool deserialize_telemetry(const uint8_t *buffer, size_t message_length, Telemetry *telemetry) {
    pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);
    if (!pb_decode(&stream, Telemetry_fields, telemetry)) {
        fprintf(stderr, "Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    return true;
}


namespace json = ArduinoJson;

class tcp_server_telemetry : public tcp_server {
public:
  tcp_server_telemetry(int port) : tcp_server("telemetry", port) {}

  void reply_fn(int sock) override {
    const int buf_len = 1024;
    uint8_t tx_buffer[buf_len];
    json::MyJsonDocument ans;    

    int times = 0;

    while (true) {
      x_y_axes->first_axis->read_pos_from_encoder();
      x_y_axes->second_axis->read_pos_from_encoder();
      z_dummy_axes->first_axis->read_pos_from_encoder();

      Telemetry telemetry = Telemetry_init_zero;
      telemetry.coords.x = x_y_axes->first_axis->current_counts / static_cast<double>(x_y_axes->first_axis->inches_to_counts_factor);
      telemetry.coords.y = x_y_axes->second_axis->current_counts / static_cast<double>(x_y_axes->second_axis->inches_to_counts_factor);
      telemetry.coords.z = z_dummy_axes->first_axis->current_counts / static_cast<double>(z_dummy_axes->first_axis->inches_to_counts_factor);
      telemetry.has_coords = true;

      telemetry.targets.x = x_y_axes->first_axis->destination_counts / static_cast<double>(x_y_axes->first_axis->inches_to_counts_factor);
      telemetry.targets.y = x_y_axes->second_axis->destination_counts / static_cast<double>(x_y_axes->second_axis->inches_to_counts_factor);
      telemetry.targets.z = z_dummy_axes->first_axis->destination_counts / static_cast<double>(z_dummy_axes->first_axis->inches_to_counts_factor);
      telemetry.has_targets = true;

      struct limits limits = encoders->read_limits();

      telemetry.limits.left = static_cast<bool>(limits.hard & 1 << 0);
      telemetry.limits.right = static_cast<bool>(limits.hard & 1 << 1);
      telemetry.limits.up = static_cast<bool>(limits.hard & 1 << 2);
      telemetry.limits.down = static_cast<bool>(limits.hard & 1 << 3);
      telemetry.limits.in = static_cast<bool>(limits.hard & 1 << 4);
      telemetry.limits.out = static_cast<bool>(limits.hard & 1 << 5);
      telemetry.limits.probe = static_cast<bool>(limits.hard & 1 << 6);
      telemetry.has_limits = true;
      
      telemetry.control_enabled = rema::control_enabled;
      telemetry.stall_control = rema::stall_control;
      telemetry.brakes_mode = static_cast<int>(rema::brakes_mode);

      telemetry.stalled.x = x_y_axes->first_axis->stalled;
      telemetry.stalled.y = x_y_axes->second_axis->stalled;
      telemetry.stalled.z = z_dummy_axes->first_axis->stalled;

      telemetry.probe.x_y = x_y_axes->was_stopped_by_probe;
      telemetry.probe.z =  z_dummy_axes->was_stopped_by_probe;
      telemetry.probe_protected =  x_y_axes->was_stopped_by_probe_protection || z_dummy_axes->was_stopped_by_probe_protection;

      telemetry.on_condition.x_y =
        (x_y_axes->already_there &&
          !x_y_axes
              ->was_soft_stopped); // Soft stops are only sent by joystick,
                                    // so no ON_CONDITION reported
      telemetry.on_condition.z =
        (z_dummy_axes->already_there && !z_dummy_axes->was_soft_stopped);
      
      telemetry.has_on_condition = true;


      if (!(times % 50)) {
        telemetry.temps.x = 
            (static_cast<double>(temperature_ds18b20_get(0))) / 10;
        telemetry.temps.y =
            (static_cast<double>(temperature_ds18b20_get(1))) / 10;
        telemetry.temps.z =
            (static_cast<double>(temperature_ds18b20_get(2))) / 10;
        telemetry.has_temps = true;
      } else {
        telemetry.has_temps = false;
      }
      times++;

      size_t msg_len;
      if (!serialize_telemetry(&telemetry, tx_buffer, sizeof(tx_buffer), &msg_len)) {
        lDebug(Error, "NANOPB Serialize error");
      }
      
      tx_buffer[msg_len] = '\0';  //null terminate
      msg_len++;     

      lDebug(InfoLocal, "To send %d bytes: %s", msg_len, tx_buffer);

      if (msg_len > 0) {
        // send() can return less bytes than supplied length.
        // Walk-around for robust implementation.
        int to_write = msg_len;
        while (to_write > 0) {
          int written =
              lwip_send(sock, tx_buffer + (msg_len - to_write), to_write, 0);
          if (written < 0) {
            lDebug(Error, "Error occurred during sending telemetry: errno %d",
                   errno);
            return;
          }
          to_write -= written;
        }
      } else {
          lDebug(Error, "buffer too small");
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
};

#endif /* TCP_SERVER_TELEMETRY_H_ */
