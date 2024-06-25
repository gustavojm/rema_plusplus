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
#include "parson.h"
#include "rema.h"
#include "tcp_server.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"

extern bresenham *x_y_axes, *z_dummy_axes;
extern encoders_pico *encoders;

class tcp_server_telemetry : public tcp_server {
public:
  tcp_server_telemetry(int port) : tcp_server("telemetry", port) {}

  void reply_fn(int sock) override {
    const int buf_len = 1024;
    char tx_buffer[buf_len];
    JSON_Value *ans = json_value_init_object();
    JSON_Value *telemetry = json_value_init_object();

    int times = 0;

    while (true) {
      x_y_axes->first_axis->read_pos_from_encoder();
      x_y_axes->second_axis->read_pos_from_encoder();
      z_dummy_axes->first_axis->read_pos_from_encoder();
      JSON_Value *coords = json_value_init_object();
      json_object_set_number(
          json_value_get_object(coords), "x",
          x_y_axes->first_axis->current_counts /
              static_cast<double>(
                  x_y_axes->first_axis->inches_to_counts_factor));
      json_object_set_number(
          json_value_get_object(coords), "y",
          x_y_axes->second_axis->current_counts /
              static_cast<double>(
                  x_y_axes->second_axis->inches_to_counts_factor));
      json_object_set_number(
          json_value_get_object(coords), "z",
          z_dummy_axes->first_axis->current_counts /
              static_cast<double>(
                  z_dummy_axes->first_axis->inches_to_counts_factor));

      json_object_set_value(json_value_get_object(telemetry), "coords", coords);

      struct limits limits = encoders->read_limits();

      JSON_Value *limits_json = json_value_init_object();
      json_object_set_boolean(json_value_get_object(limits_json), "left",
                              (limits.hard & 1 << 0));
      json_object_set_boolean(json_value_get_object(limits_json), "right",
                              (limits.hard & 1 << 1));
      json_object_set_boolean(json_value_get_object(limits_json), "up",
                              (limits.hard & 1 << 2));
      json_object_set_boolean(json_value_get_object(limits_json), "down",
                              (limits.hard & 1 << 3));
      json_object_set_boolean(json_value_get_object(limits_json), "in",
                              (limits.hard & 1 << 4));
      json_object_set_boolean(json_value_get_object(limits_json), "out",
                              (limits.hard & 1 << 5));
      json_object_set_boolean(json_value_get_object(limits_json), "probe",
                              (limits.hard & 1 << 6));
      json_object_set_value(json_value_get_object(telemetry), "limits",
                            limits_json);

      json_object_set_boolean(json_value_get_object(telemetry), "control_enabled",
                            rema::control_enabled);

      json_object_set_boolean(json_value_get_object(telemetry), "stall_control",
                            rema::stall_control_get());

      json_object_set_number(json_value_get_object(telemetry), "brakes_mode",
                            static_cast<int>(rema::brakes_mode));

      JSON_Value *stalled = json_value_init_object();
      json_object_set_boolean(json_value_get_object(stalled), "x",
                              x_y_axes->first_axis->stalled);
      json_object_set_boolean(json_value_get_object(stalled), "y",
                              x_y_axes->second_axis->stalled);
      json_object_set_boolean(json_value_get_object(stalled), "z",
                              z_dummy_axes->first_axis->stalled);
      json_object_set_value(json_value_get_object(telemetry), "stalled",
                            stalled);

      JSON_Value *probe = json_value_init_object();
      json_object_set_boolean(json_value_get_object(probe), "x_y",
                              x_y_axes->was_stopped_by_probe);
      json_object_set_boolean(json_value_get_object(probe), "z",
                              z_dummy_axes->was_stopped_by_probe);
      json_object_set_value(json_value_get_object(telemetry), "probe", probe);

      JSON_Value *on_condition = json_value_init_object();
      json_object_set_boolean(
          json_value_get_object(on_condition), "x_y",
          (x_y_axes->already_there &&
           !x_y_axes
                ->was_soft_stopped)); // Soft stops are only sent by joystick,
                                      // so no ON_CONDITION reported
      json_object_set_boolean(
          json_value_get_object(on_condition), "z",
          (z_dummy_axes->already_there && !z_dummy_axes->was_soft_stopped));
      json_object_set_value(json_value_get_object(telemetry), "on_condition",
                            on_condition);

      if (!(times % 50)) {
        JSON_Value *temperatures = json_value_init_object();
        json_object_set_number(
            json_value_get_object(temperatures), "x",
            (static_cast<double>(temperature_ds18b20_get(0))) / 10);
        json_object_set_number(
            json_value_get_object(temperatures), "y",
            (static_cast<double>(temperature_ds18b20_get(1))) / 10);
        json_object_set_number(
            json_value_get_object(temperatures), "z",
            (static_cast<double>(temperature_ds18b20_get(2))) / 10);
        json_object_set_value(json_value_get_object(ans), "temps",
                              temperatures);
      } else {
        json_object_remove(json_value_get_object(ans), "temps");
      }
      times++;

      json_object_set_value(json_value_get_object(ans), "telemetry", telemetry);

      int msg_len = json_serialize_to_buffer_return_written(ans, tx_buffer, buf_len - 1);
      tx_buffer[msg_len] = '\0';  //null terminate
      msg_len++; 

      //lDebug(InfoLocal, "To send %d bytes: %s", msg_len, tx_buffer);

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
            goto err_send;
          }
          to_write -= written;
        }
      } else {
          lDebug(Error, "buffer too small");
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  err_send:
    json_value_free(ans);
  }
};

#endif /* TCP_SERVER_TELEMETRY_H_ */
