//#include <stdlib.h>
#include "FreeRTOS.h"
#include "debug.h"
#include <cctype>
#include <memory>
#include <stdio.h>
#include <string.h>

#include "bresenham.h"
#include "encoders_pico.h"
#include "mot_pap.h"
#include "rema.h"
#include "settings.h"
#include "tcp_server_command.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"
#include "debug.h"

#define PROTOCOL_VERSION "JSON_1.0"

extern encoders_pico *encoders;

namespace json = ArduinoJson;

bresenham *tcp_server_command::get_axes(const char *axis) {

  switch (*axis) {
  case 'z':
  case 'Z':
    return z_dummy_axes;
    break;
  default:
    return x_y_axes;
    break;
  }
}

json::MyJsonDocument check_control_and_brakes(bresenham *axes) {
  json::MyJsonDocument res;
    if (!rema::control_enabled_get()) {
        res["ERROR"] = "CONTROL IS DISABLED";
        return res;
    }

    if (axes->has_brakes && rema::brakes_mode == rema::brakes_mode_t::ON) {
        res["ERROR"] = "BRAKES ARE APPLIED";
        return res;        
    }

    return res;  // Indicating no errors
}

json::MyJsonDocument tcp_server_command::set_log_level_cmd(json::JsonObject pars) {
  json::MyJsonDocument root_value;

  if (pars.containsKey("local_level")) {
    char const *level = pars["local_level"];

    if (!strcmp(level, "Debug")) {
      debugLocalSetLevel(true, Debug);
    }

    if (!strcmp(level, "Info")) {
      debugLocalSetLevel(true, Info);
    }

    if (!strcmp(level, "Warning")) {
      debugLocalSetLevel(true, Warn);
    }

    if (!strcmp(level, "Error")) {
      debugLocalSetLevel(true, Error);
    }
  }

  if (pars.containsKey("net_level")) {
    char const *net_level = pars["net_level"];

    if (!strcmp(net_level, "Debug")) {
      debugNetSetLevel(true, Debug);
    }

    if (!strcmp(net_level, "Info")) {
      debugNetSetLevel(true, Info);
    }

    if (!strcmp(net_level, "Warning")) {
      debugNetSetLevel(true, Warn);
    }

    if (!strcmp(net_level, "Error")) {
      debugNetSetLevel(true, Error);
    }
  }

  return root_value;
}

json::MyJsonDocument tcp_server_command::logs_cmd(json::JsonObject const pars) {
    double quantity = pars["quantity"];

    json::MyJsonDocument root_value;
    auto msg_array = root_value["DEBUG_MSGS"].to<json::JsonArray>();
    int msgs_waiting = uxQueueMessagesWaiting(debug_queue);
    int extract = MIN(quantity, msgs_waiting);

    for (int x = 0; x < extract; x++) {
      char *dbg_msg = NULL;
      if (xQueueReceive(debug_queue, &dbg_msg, (TickType_t)0) == pdPASS) {
        msg_array.add(dbg_msg);
        delete[] dbg_msg;
        dbg_msg = NULL;
      }
    }

    return root_value;  
}

json::MyJsonDocument tcp_server_command::protocol_version_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  root_value["Version"] = PROTOCOL_VERSION;
  return root_value;
}

json::MyJsonDocument tcp_server_command::control_enable_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;

  if (pars.containsKey("enabled")) {
    bool enabled = pars["enabled"];
    rema::control_enabled_set(enabled);
  }
  root_value["STATUS"] = rema::control_enabled_get();
  return root_value;
}

json::MyJsonDocument tcp_server_command::brakes_mode_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  if (pars.containsKey("mode")) {
    char const *mode = pars["mode"];

    if (!strcmp(mode, "OFF")) {
      rema::brakes_mode = rema::brakes_mode_t::OFF;
      rema::brakes_release();
    }

    if (!strcmp(mode, "AUTO")) {
      rema::brakes_mode = rema::brakes_mode_t::AUTO;
    }

    if (!strcmp(mode, "ON")) {
      rema::brakes_mode = rema::brakes_mode_t::ON;
      rema::brakes_apply();
    }
  }

  switch (rema::brakes_mode) {
  case rema::brakes_mode_t::OFF:
    root_value["STATUS"] = "OFF";
    break;

  case rema::brakes_mode_t::AUTO:
    root_value["STATUS"] = "AUTO";
    break;

  case rema::brakes_mode_t::ON:
    root_value["STATUS"] = "ON";
    break;

  default:
    break;
  }

  return root_value;
}

json::MyJsonDocument tcp_server_command::touch_probe_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  if (pars.containsKey("position")) {
    char const *position = pars["position"];

    if (!strcmp(position, "IN")) {
      rema::touch_probe_retract();
    }

    if (!strcmp(position, "OUT")) {
      rema::touch_probe_extend();
    }
  }

  return root_value;
}

json::MyJsonDocument tcp_server_command::stall_control_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  if (pars.containsKey("enabled")) {
    bool enabled = pars["enabled"];
    rema::stall_control_set(enabled);
  }
  root_value["STATUS"] = rema::stall_control_get();
  return root_value;
}

json::MyJsonDocument tcp_server_command::set_coords_cmd(json::JsonObject const pars) {
    if (pars.containsKey("position_x")) {
      double pos_x = pars["position_x"];
      x_y_axes->first_axis->set_position(pos_x);
    }

    if (pars.containsKey("position_y")) {
      double pos_y = pars["position_y"];
      x_y_axes->second_axis->set_position(pos_y);
    }

    if (pars.containsKey("position_z")) {
      double pos_z = pars["position_z"];
      z_dummy_axes->first_axis->set_position(pos_z);
    }
  json::MyJsonDocument root_value;
  root_value["ACK"] =  true;
  return root_value;
}

json::MyJsonDocument tcp_server_command::kp_set_tunings_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  char const *axes = pars["axes"];
  double kp = pars["kp"];
  int update = pars["update"];
  int min = pars["min"];
  int max = pars["max"];

  bresenham *axes_ = get_axes(axes);

  if (axes_ == nullptr) {
    root_value["ACK"] = false;
    root_value["ERROR"] = "No axis specified";
  } else {
    axes_->step_time = std::chrono::milliseconds(update);
    axes_->kp.set_output_limits(min, max);
    axes_->kp.set_sample_period(axes_->step_time);
    axes_->kp.set_tunings(kp);
    lDebug(Debug, "KP Settings set");
    root_value["ACK"] = true;
  }
  return root_value;
}

json::MyJsonDocument tcp_server_command::axes_hard_stop_all_cmd(json::JsonObject const pars) {
  x_y_axes->send({mot_pap::HARD_STOP});
  z_dummy_axes->send({mot_pap::HARD_STOP});

  json::MyJsonDocument root_value;
  root_value["ACK"] = true;
  return root_value;
}

json::MyJsonDocument tcp_server_command::axes_soft_stop_all_cmd(json::JsonObject const pars) {
  x_y_axes->send({mot_pap::SOFT_STOP});
  z_dummy_axes->send({mot_pap::SOFT_STOP});
  json::MyJsonDocument root_value;
  root_value["ACK"] = true;
  return root_value;
}

json::MyJsonDocument tcp_server_command::network_settings_cmd(json::JsonObject const pars) {
  char const *ipaddr = pars["ipaddr"];
  char const *netmask = pars["netmask"];
  char const *gw = pars["gw"];
  uint16_t port = static_cast<uint16_t>(pars["port"]);

  if (gw && ipaddr && netmask && port != 0) {
    lDebug(Info,
            "Received network settings: ipaddr:%s, netmask:%s, gw:%s, port:%d",
            ipaddr, netmask, gw, port);

    int octet1, octet2, octet3, octet4;
    unsigned char *ipaddr_bytes =
        reinterpret_cast<unsigned char *>(&(settings::network.ipaddr.addr));

    if (sscanf(ipaddr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) ==
        4) {
      ipaddr_bytes[0] = static_cast<unsigned char>(octet1);
      ipaddr_bytes[1] = static_cast<unsigned char>(octet2);
      ipaddr_bytes[2] = static_cast<unsigned char>(octet3);
      ipaddr_bytes[3] = static_cast<unsigned char>(octet4);
    }

    unsigned char *netmask_bytes =
        reinterpret_cast<unsigned char *>(&(settings::network.netmask.addr));
    if (sscanf(netmask, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) ==
        4) {
      netmask_bytes[0] = static_cast<unsigned char>(octet1);
      netmask_bytes[1] = static_cast<unsigned char>(octet2);
      netmask_bytes[2] = static_cast<unsigned char>(octet3);
      netmask_bytes[3] = static_cast<unsigned char>(octet4);
    }

    unsigned char *gw_bytes =
        reinterpret_cast<unsigned char *>(&(settings::network.gw.addr));
    if (sscanf(gw, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) == 4) {
      gw_bytes[0] = static_cast<unsigned char>(octet1);
      gw_bytes[1] = static_cast<unsigned char>(octet2);
      gw_bytes[2] = static_cast<unsigned char>(octet3);
      gw_bytes[3] = static_cast<unsigned char>(octet4);
    }

    settings::network.port = port;

    settings::save();
    lDebug(Info, "Settings saved. Restarting...");

    Chip_UART_SendBlocking(DEBUG_UART, "\n\n", 2);

    Chip_RGU_TriggerReset(RGU_CORE_RST);
  }

  json::MyJsonDocument root_value;
  root_value["ACK"] = true;
  return root_value;
}

json::MyJsonDocument tcp_server_command::mem_info_cmd(json::JsonObject const pars) {
  auto root_value = json::MyJsonDocument();
  root_value["MEM_TOTAL"] = configTOTAL_HEAP_SIZE;
  root_value["MEM_FREE"] = xPortGetFreeHeapSize();
  root_value["MEM_MIN_FREE"] = xPortGetMinimumEverFreeHeapSize();
  return root_value;
}

json::MyJsonDocument tcp_server_command::temperature_info_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  root_value["TEMP X"] = static_cast<double>(temperature_ds18b20_get(0)) / 10;
  root_value["TEMP Y"] = static_cast<double>(temperature_ds18b20_get(1)) / 10;
  root_value["TEMP Z"] = static_cast<double>(temperature_ds18b20_get(2)) / 10;
  return root_value;
}

json::MyJsonDocument tcp_server_command::move_closed_loop_cmd(json::JsonObject const pars) {  
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);

    auto  error = check_control_and_brakes(axes_);
    if (!error.isNull()) {
      return error;
    }

    double first_axis_setpoint = pars["first_axis_setpoint"];
    double second_axis_setpoint = pars["second_axis_setpoint"];

    bresenham_msg msg;
    msg.type = mot_pap::type::MOVE;
    msg.first_axis_setpoint = static_cast<int>(
        first_axis_setpoint * axes_->first_axis->inches_to_counts_factor);
    msg.second_axis_setpoint = static_cast<int>(
        second_axis_setpoint * axes_->second_axis->inches_to_counts_factor);

    axes_->send(msg);

    lDebug(Info, "MOVE_CLOSED_LOOP First Axis Setpoint= %f, Second Axis Setpoint= %f",
           first_axis_setpoint, second_axis_setpoint);

    json::MyJsonDocument root_value;
    root_value["ACK"] = true;
    return root_value;
}

json::MyJsonDocument tcp_server_command::move_joystick_cmd(json::JsonObject const pars) {
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);

    auto error = check_control_and_brakes(axes_);
    if (!error.isNull()) {
      return error;
    }

    int first_axis_setpoint, second_axis_setpoint;
    if (pars.containsKey("first_axis_setpoint")) {
      first_axis_setpoint = static_cast<int>(pars["first_axis_setpoint"]);
    } else {
      first_axis_setpoint = axes_->first_axis->current_counts;
    }

    if (pars.containsKey("second_axis_setpoint")) {
      second_axis_setpoint = static_cast<int>(pars["second_axis_setpoint"]);
    } else {
      second_axis_setpoint = axes_->second_axis->current_counts;
    }

    bresenham_msg msg;
    msg.type = mot_pap::type::MOVE;
    msg.first_axis_setpoint = first_axis_setpoint;
    msg.second_axis_setpoint = second_axis_setpoint;
    axes_->send(msg);
    // lDebug(Info, "MOVE_JOYSTICK First Axis Setpoint= %i, Second Axis Setpoint= %i",
    //        first_axis_setpoint, second_axis_setpoint);

    json::MyJsonDocument root_value;
    root_value["ACK"] = true;
    return root_value;
  }

 json::MyJsonDocument tcp_server_command::move_incremental_cmd(json::JsonObject const pars) {  
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);
    
    auto error = check_control_and_brakes(axes_);
    if (!error.isNull()) {
      return error;
    }

    double first_axis_delta, second_axis_delta;
    if (pars.containsKey("first_axis_delta")) {
      first_axis_delta = pars["first_axis_delta"];
    } else {
      first_axis_delta = 0;
    }

    if (pars.containsKey("second_axis_delta")) {
      second_axis_delta =
          pars["second_axis_delta"];
    } else {
      second_axis_delta = 0;
    }

    bresenham_msg msg;
    msg.type = mot_pap::type::MOVE;
    msg.first_axis_setpoint =
        axes_->first_axis->current_counts +
        (first_axis_delta * axes_->first_axis->inches_to_counts_factor);
    msg.second_axis_setpoint =
        axes_->second_axis->current_counts +
        (second_axis_delta * axes_->first_axis->inches_to_counts_factor);
    axes_->send(msg);
    // lDebug(Info, "MOVE_INCREMENTAL First Axis Setpoint= %i, Second Axis Setpoint= %i",
    //        msg.first_axis_setpoint, msg.second_axis_setpoint);  
    json::MyJsonDocument root_value;
    root_value["ACK"] = true;
    return root_value;
}

json::MyJsonDocument tcp_server_command::read_encoders_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  
  if (pars.containsKey("axis")) {
    char const *axis = pars["axis"];
    root_value[axis] = encoders->read_counter(axis[0]);
    return root_value;
  } else {

    uint8_t rx[4 * 3] = {0x00};
    encoders->read_counters(rx);
    int32_t x = (rx[0] << 24 | rx[1] << 16 | rx[2] << 8 | rx[3] << 0);
    int32_t y = (rx[4] << 24 | rx[5] << 16 | rx[6] << 8 | rx[7] << 0);
    int32_t z = (rx[8] << 24 | rx[9] << 16 | rx[10] << 8 | rx[11] << 0);
    int32_t w [[maybe_unused]] =
        (rx[12] << 24 | rx[13] << 16 | rx[14] << 8 | rx[15] << 0);

    root_value["X"] = x;
    root_value["Y"] = y;
    root_value["Z"] = z;
    return root_value;
  }
}

json::MyJsonDocument tcp_server_command::read_limits_cmd(json::JsonObject const pars) {
  json::MyJsonDocument root_value;
  root_value["ACK"] = encoders->read_limits().hard;
  return root_value;
}

// @formatter:off
const tcp_server_command::cmd_entry tcp_server_command::cmds_table[] = {
    {
        "PROTOCOL_VERSION",                        /* Command name */
        &tcp_server_command::protocol_version_cmd, /* Associated function */
    },
    {
        "CONTROL_ENABLE",
        &tcp_server_command::control_enable_cmd,
    },
    {
        "STALL_CONTROL",
        &tcp_server_command::stall_control_cmd,
    },
    {
        "BRAKES_MODE",
        &tcp_server_command::brakes_mode_cmd,
    },
    {
        "TOUCH_PROBE",
        &tcp_server_command::touch_probe_cmd,
    },
    {
        "AXES_HARD_STOP_ALL",
        &tcp_server_command::axes_hard_stop_all_cmd,
    },
    {
        "AXES_SOFT_STOP_ALL",
        &tcp_server_command::axes_soft_stop_all_cmd,
    },
    {
        "LOGS",
        &tcp_server_command::logs_cmd,
    },
    {
        "SET_LOG_LEVEL",
        &tcp_server_command::set_log_level_cmd,
    },

    {
        "KP_SET_TUNINGS",
        &tcp_server_command::kp_set_tunings_cmd,
    },
    {
        "NETWORK_SETTINGS",
        &tcp_server_command::network_settings_cmd,
    },
    {
        "MEM_INFO",
        &tcp_server_command::mem_info_cmd,
    },
    {
        "TEMP_INFO",
        &tcp_server_command::temperature_info_cmd,
    },
    {
        "SET_COORDS",
        &tcp_server_command::set_coords_cmd,
    },
    {
        "MOVE_JOYSTICK",
        &tcp_server_command::move_joystick_cmd,
    },
    {
        "MOVE_CLOSED_LOOP",
        &tcp_server_command::move_closed_loop_cmd,
    },
    {
        "MOVE_INCREMENTAL",
        &tcp_server_command::move_incremental_cmd,
    },
    {
        "READ_ENCODERS",
        &tcp_server_command::read_encoders_cmd,
    },
    {
        "READ_LIMITS",
        &tcp_server_command::read_limits_cmd,
    },
};
// @formatter:on

/**
 * @brief 	searchs for a matching command name in cmds_table[], passing the
 * parameters as a JSON object for the called function to parse them.
 * @param 	*cmd 	:name of the command to execute
 * @param   *pars   :JSON object containing the passed parameters to the called
 * function
 */
json::MyJsonDocument tcp_server_command::cmd_execute(char const *cmd, json::JsonObject const pars) {
  bool cmd_found = false;
  for (unsigned int i = 0; i < (sizeof(cmds_table) / sizeof(cmds_table[0]));
       i++) {
    if (!strcmp(cmd, cmds_table[i].cmd_name)) {
      // return cmds_table[i].cmd_function(pars);
      return (this->*(cmds_table[i].cmd_function))(pars);
    }
  }
  if (!cmd_found) {
    lDebug(Error, "No matching command found");
  }
  return json::MyJsonDocument();
};

/**
 * @brief Defines a simple wire protocol base on JavaScript Object Notation
 (JSON)
 * @details Receives an JSON array of commands
 * Every command entry in this array must be a JSON object,
 containing
 * a "command" string specifying the name of the called command, and a nested
 JSON object
 * with the parameters for the called command under the "pars" key.
 * The called function must extract the parameters from the passed JSON object.
 *
 * Example of the received JSON object:
 *
 * [
      {"cmd": "MOVE_JOYSTICK",
       "pars": {
          "first_axis_setpoint": 500,
          "second_axis_setpoint": 100
       }
      },
      {"cmd": "LOGS",
       "pars": {
          "quantity": 10
       }
      }
   ]

 * Every executed command has the chance of returning a JSON object that will be
 inserted
 * in the response JSON object under a key corresponding to the executed command
 name, or
 * NULL if no answer is expected.
 */

/**
 * @brief 	Parses the received JSON object looking for commands to execute
 * and appends the outputs of the called commands to the response buffer.
 * @param 	*rx_buff 	:pointer to the received buffer from the network
 * @param   **tx_buff	:pointer to pointer, will be set to the allocated return
 * buffer
 * @returns	the length of the allocated response buffer
 */
int tcp_server_command::json_wp(char *rx_buff, char **tx_buff) {
  auto rx_JSON_value = json::MyJsonDocument();
  json::DeserializationError error = json::deserializeJson(rx_JSON_value,  rx_buff);

  auto tx_JSON_value = json::MyJsonDocument();
  *tx_buff = NULL;
  int buff_len = 0;

  if (error) {
      lDebug(Error, "Error json parse. %s", error.c_str());
  } else {
    for (json::JsonVariant command : rx_JSON_value.as<json::JsonArray>()) {
      char const *command_name = command["cmd"];
      lDebug(InfoLocal, "Command Found: %s", command_name);
      auto pars = command["pars"];

      auto ans = cmd_execute(command_name, pars);
      //if (ans) {
      tx_JSON_value[command_name] = ans;
      //}
    }
    
    buff_len = json::measureJson(tx_JSON_value); /* returns 0 on fail */
    buff_len ++;
    *tx_buff = new char[buff_len];
    if (!(*tx_buff)) {
      lDebug(Error, "Out Of Memory");
      buff_len = 0;
    } else {    
      json::serializeJson(tx_JSON_value, *tx_buff, buff_len);
      lDebug(InfoLocal, "%s", *tx_buff);
    }
  }
  return buff_len;
}
