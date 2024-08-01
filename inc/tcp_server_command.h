#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "arduinojson_cust_alloc.h"
#include "debug.h"
#include "rema.h"
#include "tcp_server.h"
#include "xy_axes.h"
#include "z_axis.h"

namespace json = ArduinoJson;

class tcp_server_command : public tcp_server {
public:
  tcp_server_command(int port) : tcp_server("command", port) {}

  static bresenham *get_axes(const char *axis);

  void stop_all() {
    x_y_axes->stop();
    z_dummy_axes->stop();
    lDebug(Warn, "Stopping all");
  }

  void reply_fn(int sock) override {
    int len;
    char rx_buffer[1024];

    do {
      len = lwip_recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
      if (len < 0) {
        stop_all();
        lDebug(Error, "Error occurred during receiving command: errno %d",
               errno);
      } else if (len == 0) {
        stop_all();
        lDebug(Warn, "Command connection closed");
      } else {
        // rema::update_watchdog_timer();
        rx_buffer[len] =
            0; // Null-terminate whatever is received and treat it like a string
        lDebug(InfoLocal, "Command received %s", rx_buffer);

        char *tx_buffer;

        int ack_len = json_wp(rx_buffer, &tx_buffer);

        // lDebug(InfoLocal, "To send %d bytes: %s", ack_len, tx_buffer);

        if (ack_len > 0) {
          // send() can return less bytes than supplied length.
          // Walk-around for robust implementation.
          int to_write = ack_len;

          while (to_write > 0) {
            int written =
                lwip_send(sock, tx_buffer + (ack_len - to_write), to_write, 0);
            if (written < 0) {
              stop_all();
              lDebug(Error, "Error occurred during sending command: errno %d",
                     errno);
              goto free_buffer;
            }
            to_write -= written;
          }

        free_buffer:
          if (tx_buffer) {
            delete[] tx_buffer;
            tx_buffer = NULL;
          }
        }
      }
    } while (len > 0);
  }

  json::MyJsonDocument logs_cmd(json::JsonObject const pars);
  json::MyJsonDocument set_log_level_cmd(json::JsonObject const pars);
  json::MyJsonDocument protocol_version_cmd(json::JsonObject const pars);
  json::MyJsonDocument control_enable_cmd(json::JsonObject const pars);
  json::MyJsonDocument stall_control_cmd(json::JsonObject const pars);
  json::MyJsonDocument
  touch_probe_protection_control_cmd(json::JsonObject const pars);
  json::MyJsonDocument set_coords_cmd(json::JsonObject const pars);
  json::MyJsonDocument kp_set_tunings_cmd(json::JsonObject const pars);
  json::MyJsonDocument axes_hard_stop_all_cmd(json::JsonObject const pars);
  json::MyJsonDocument axes_soft_stop_all_cmd(json::JsonObject const pars);
  json::MyJsonDocument network_settings_cmd(json::JsonObject const pars);
  json::MyJsonDocument mem_info_cmd(json::JsonObject const pars);
  json::MyJsonDocument temperature_info_cmd(json::JsonObject const pars);
  json::MyJsonDocument move_closed_loop_cmd(json::JsonObject const pars);
  json::MyJsonDocument move_joystick_cmd(json::JsonObject const pars);
  json::MyJsonDocument move_incremental_cmd(json::JsonObject const pars);
  json::MyJsonDocument brakes_mode_cmd(json::JsonObject const pars);
  json::MyJsonDocument touch_probe_cmd(json::JsonObject const pars);
  json::MyJsonDocument read_encoders_cmd(json::JsonObject const pars);
  json::MyJsonDocument read_limits_cmd(json::JsonObject const pars);
  json::MyJsonDocument cmd_execute(char const *cmd,
                                   json::JsonObject const pars);

  int json_wp(char *rx_buff, char **tx_buff);

  // FredMemFn points to a member of Fred that takes (char,float)
  typedef json::MyJsonDocument (tcp_server_command::*cmd_function_ptr)(
      json::JsonObject pars);

  typedef struct {
    const char *cmd_name;
    cmd_function_ptr cmd_function;
  } cmd_entry;

  static const cmd_entry cmds_table[];
};
