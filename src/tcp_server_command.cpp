//#include <stdlib.h>
#include "FreeRTOS.h"
#include "debug.h"
#include <cctype>
#include <memory>
#include <stdio.h>
#include <string.h>

#include "bresenham.h"
#include "debug.h"
#include "encoders_pico.h"
#include "expected.hpp"
#include "mot_pap.h"
#include "rema.h"
#include "settings.h"
#include "tcp_server_command.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"
#include "ip_fns.h"

#define PROTOCOL_VERSION "JSON_1.0"

namespace json = ArduinoJson;

bresenham *tcp_server_command::get_axes(const char *axis) {

    switch (*axis) {
    case 'z':
    case 'Z': return z_dummy_axes; break;
    default: return x_y_axes; break;
    }
}

tl::expected<void, const char *> check_control_and_brakes(bresenham *axes) {
    if (!rema::control_enabled_get()) {
        return tl::make_unexpected("Control is disabled");
    }

    if (axes->has_brakes && rema::brakes_mode == rema::brakes_mode_t::ON) {
        return tl::make_unexpected("Brakes are applied");
    }

    return {}; // Indicating no errors
}

json::MyJsonDocument tcp_server_command::log_level_cmd(json::JsonObject pars) {
    json::MyJsonDocument res;

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

    res["local_level"] = levelText(debugLocalLevel);
    res["net_level"] = levelText(debugNetLevel);
    return res;
}

json::MyJsonDocument tcp_server_command::logs_cmd(json::JsonObject const pars) {
    double quantity = pars["quantity"];

    json::MyJsonDocument res;
    auto msg_array = res["DEBUG_MSGS"].to<json::JsonArray>();
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

    return res;
}

json::MyJsonDocument tcp_server_command::protocol_version_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    res["version"] = PROTOCOL_VERSION;
    return res;
}

json::MyJsonDocument tcp_server_command::control_enable_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;

    if (pars.containsKey("enabled")) {
        bool enabled = pars["enabled"];
        rema::control_enabled_set(enabled);
    }
    res["status"] = rema::control_enabled_get();
    return res;
}

json::MyJsonDocument tcp_server_command::brakes_mode_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
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
    case rema::brakes_mode_t::OFF: res["status"] = "OFF"; break;

    case rema::brakes_mode_t::AUTO: res["status"] = "AUTO"; break;

    case rema::brakes_mode_t::ON: res["status"] = "ON"; break;

    default: break;
    }

    return res;
}

json::MyJsonDocument tcp_server_command::touch_probe_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    if (pars.containsKey("position")) {
        char const *position = pars["position"];

        if (!strcmp(position, "IN")) {
            rema::touch_probe_retract();
        }

        if (!strcmp(position, "OUT")) {
            rema::touch_probe_extend();
        }
    }

    return res;
}

json::MyJsonDocument tcp_server_command::stall_control_settings_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    if (pars.containsKey("enabled")) {
        rema::stall_control = pars["enabled"];
    }

    if (pars.containsKey("counts_X")) {
        x_y_axes->first_axis->stall_max_count = pars["counts_X"];;
    }

    if (pars.containsKey("counts_Y")) {
        x_y_axes->second_axis->stall_max_count = pars["counts_Y"];;
    }

    if (pars.containsKey("counts_Z")) {
        z_dummy_axes->first_axis->stall_max_count= pars["counts_Z"];
    }

    res["status"] = rema::stall_control;
    res["counts_X"] = x_y_axes->first_axis->stall_max_count;
    res["counts_Y"] = x_y_axes->second_axis->stall_max_count;
    res["counts_Z"] = z_dummy_axes->first_axis->stall_max_count;
    return res;
}

json::MyJsonDocument tcp_server_command::touch_probe_settings_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    if (pars.containsKey("protection")) {
        rema::touch_probe_protection = pars["protection"];
    }

    if (pars.containsKey("counts_XY")) {
        x_y_axes->touching_max_count = pars["counts_XY"];
    }

    if (pars.containsKey("counts_Z")) {
        z_dummy_axes->touching_max_count = pars["counts_Z"];
    }

    if (pars.containsKey("debounce_time_ms")) {
        rema::touch_probe_debounce_time_ms = pars["debounce_time_ms"];
    }

    res["protection"] = rema::touch_probe_protection;
    res["counts_XY"] = x_y_axes->touching_max_count;
    res["counts_Z"] = z_dummy_axes->touching_max_count;
    res["debounce_time_ms"] = rema::touch_probe_debounce_time_ms;
    return res;
}

json::MyJsonDocument tcp_server_command::set_coords_cmd(json::JsonObject const pars) {
    if (pars.containsKey("position_X")) {
        double pos_x = pars["position_X"];
        x_y_axes->first_axis->set_position(pos_x);
    }

    if (pars.containsKey("position_Y")) {
        double pos_y = pars["position_Y"];
        x_y_axes->second_axis->set_position(pos_y);
    }

    if (pars.containsKey("position_Z")) {
        double pos_z = pars["position_Z"];
        z_dummy_axes->first_axis->set_position(pos_z);
    }
    json::MyJsonDocument res;
    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::axes_settings_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    double prop_gain = pars["prop_gain"];
    int update = pars["update"];
    int min = pars["min"];
    int max = pars["max"];

    if (pars.containsKey("axes")) {
        char const *axes = pars["axes"];
        bresenham *axes_ = get_axes(axes);
        axes_->step_time = std::chrono::milliseconds(update);
        axes_->kp.set_output_limits(min, max);
        axes_->kp.set_sample_period(axes_->step_time);
        axes_->kp.set_tunings(prop_gain);
        lDebug_uart_semihost(Debug, "%s settings set", axes_->name);
        res["ack"] = true;
    } else {
        res["XY"]["min_freq"] = x_y_axes->kp.out_min;
        res["XY"]["max_freq"] = x_y_axes->kp.out_max;
        res["XY"]["update_time"] = x_y_axes->step_time.count();
        res["XY"]["prop_gain"] = x_y_axes->kp.kp_;

        res["Z"]["min_freq"] = z_dummy_axes->kp.out_min;
        res["Z"]["max_freq"] = z_dummy_axes->kp.out_max;
        res["Z"]["update_time"] = z_dummy_axes->step_time.count();
        res["Z"]["prop_gain"] = z_dummy_axes->kp.kp_;
    }
    return res;
}

json::MyJsonDocument tcp_server_command::axes_hard_stop_all_cmd(json::JsonObject const pars) {
    x_y_axes->send({ mot_pap::HARD_STOP });
    z_dummy_axes->send({ mot_pap::HARD_STOP });

    json::MyJsonDocument res;
    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::axes_soft_stop_all_cmd(json::JsonObject const pars) {
    x_y_axes->send({ mot_pap::SOFT_STOP });
    z_dummy_axes->send({ mot_pap::SOFT_STOP });
    json::MyJsonDocument res;
    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::network_settings_cmd(json::JsonObject const pars) {
    char const *ipaddr = pars["ipaddr"];
    char const *netmask = pars["netmask"];
    char const *gw = pars["gw"];
    uint16_t port = static_cast<uint16_t>(pars["port"]);

    if (gw && ipaddr && netmask && port != 0) {
        lDebug_uart_semihost(Info, "Received network settings: ipaddr:%s, netmask:%s, gw:%s, port:%d", ipaddr, netmask, gw, port);

        int octet1, octet2, octet3, octet4;
        if (sscanf(ipaddr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) == 4) {
            IP4_ADDR(&(settings::network.ipaddr), octet1, octet2, octet3, octet4);
        }

        if (sscanf(netmask, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) == 4) {
            IP4_ADDR(&(settings::network.netmask), octet1, octet2, octet3, octet4);
        }

        if (sscanf(gw, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4) == 4) {
            IP4_ADDR(&(settings::network.gw), octet1, octet2, octet3, octet4);
        }

        settings::network.port = port;

        settings::save();
        lDebug_uart_semihost(Info, "Settings saved. Restarting...");

        Chip_UART_SendBlocking(DEBUG_UART, "\n\n", 2);

        Chip_RGU_TriggerReset(RGU_CORE_RST);
    }

    json::MyJsonDocument res;
    char ip_dot_format[15];
    ipaddr_to_dot_format(settings::network.ipaddr, ip_dot_format);
    res["ipaddr"] = ip_dot_format;

    ipaddr_to_dot_format(settings::network.netmask, ip_dot_format);
    res["netmask"] = ip_dot_format;

    ipaddr_to_dot_format(settings::network.gw, ip_dot_format);
    res["gw"] = ip_dot_format;

    res["port"] = settings::network.port;
    return res;
}

json::MyJsonDocument tcp_server_command::mem_info_cmd(json::JsonObject const pars) {
    auto res = json::MyJsonDocument();
    res["total"] = configTOTAL_HEAP_SIZE;
    res["free"] = xPortGetFreeHeapSize();
    res["min_free"] = xPortGetMinimumEverFreeHeapSize();
    return res;
}

json::MyJsonDocument tcp_server_command::temperature_info_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    res["temp_X"] = static_cast<double>(temperature_ds18b20_get(0)) / 10;
    res["temp_Y"] = static_cast<double>(temperature_ds18b20_get(1)) / 10;
    res["temp_Z"] = static_cast<double>(temperature_ds18b20_get(2)) / 10;
    return res;
}

json::MyJsonDocument tcp_server_command::move_closed_loop_cmd(json::JsonObject const pars) {
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);
    json::MyJsonDocument res;

    auto check_result = check_control_and_brakes(axes_);
    if (!check_result) {
        res["error"] = check_result.error();
        return res;
    }

    double first_axis_setpoint = pars["first_axis_setpoint"];
    double second_axis_setpoint = pars["second_axis_setpoint"];

    bresenham_msg msg;
    msg.type = mot_pap::type::MOVE;
    msg.first_axis_setpoint = static_cast<int>(first_axis_setpoint * axes_->first_axis->inches_to_counts_factor);
    msg.second_axis_setpoint = static_cast<int>(second_axis_setpoint * axes_->second_axis->inches_to_counts_factor);

    axes_->send(msg);

    lDebug_uart_semihost(
        Info,
        "MOVE_CLOSED_LOOP First Axis Setpoint= %f, Second Axis Setpoint= %f",
        first_axis_setpoint,
        second_axis_setpoint);

    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::move_joystick_cmd(json::JsonObject const pars) {
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);
    json::MyJsonDocument res;

    auto check_result = check_control_and_brakes(axes_);
    if (!check_result) {
        res["error"] = check_result.error();
        return res;
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
    // lDebug_uart_semihost(Info, "MOVE_JOYSTICK First Axis Setpoint= %i, Second Axis Setpoint=
    // %i",
    //        first_axis_setpoint, second_axis_setpoint);

    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::move_incremental_cmd(json::JsonObject const pars) {
    char const *axes = pars["axes"];
    bresenham *axes_ = get_axes(axes);
    json::MyJsonDocument res;

    auto check_result = check_control_and_brakes(axes_);
    if (!check_result) {
        res["error"] = check_result.error();
        return res;
    }

    double first_axis_delta, second_axis_delta;
    if (pars.containsKey("first_axis_delta")) {
        first_axis_delta = pars["first_axis_delta"];
    } else {
        first_axis_delta = 0;
    }

    if (pars.containsKey("second_axis_delta")) {
        second_axis_delta = pars["second_axis_delta"];
    } else {
        second_axis_delta = 0;
    }

    bresenham_msg msg;
    msg.type = mot_pap::type::MOVE;
    msg.first_axis_setpoint =
        axes_->first_axis->current_counts + (first_axis_delta * axes_->first_axis->inches_to_counts_factor);
    msg.second_axis_setpoint =
        axes_->second_axis->current_counts + (second_axis_delta * axes_->first_axis->inches_to_counts_factor);
    axes_->send(msg);
    // lDebug_uart_semihost(Info, "MOVE_INCREMENTAL First Axis Setpoint= %i, Second Axis
    // Setpoint= %i",
    //        msg.first_axis_setpoint, msg.second_axis_setpoint);
    res["ack"] = true;
    return res;
}

json::MyJsonDocument tcp_server_command::read_encoders_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;

    if (pars.containsKey("axis")) {
        char const *axis = pars["axis"];
        res[axis] = encoders->read_counter(axis[0]);
        return res;
    } else {

        uint8_t rx[4 * 3] = { 0x00 };
        encoders->read_counters(rx);
        int32_t x = (rx[0] << 24 | rx[1] << 16 | rx[2] << 8 | rx[3] << 0);
        int32_t y = (rx[4] << 24 | rx[5] << 16 | rx[6] << 8 | rx[7] << 0);
        int32_t z = (rx[8] << 24 | rx[9] << 16 | rx[10] << 8 | rx[11] << 0);
        int32_t w [[maybe_unused]] = (rx[12] << 24 | rx[13] << 16 | rx[14] << 8 | rx[15] << 0);

        res["X"] = x;
        res["Y"] = y;
        res["Z"] = z;
        return res;
    }
}

json::MyJsonDocument tcp_server_command::read_limits_cmd(json::JsonObject const pars) {
    json::MyJsonDocument res;
    res["ack"] = encoders->read_limits().hard;
    return res;
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
        "BRAKES_MODE",
        &tcp_server_command::brakes_mode_cmd,
    },
    {
        "TOUCH_PROBE",
        &tcp_server_command::touch_probe_cmd,
    },
    {
        "STALL_CONTROL_SETTINGS",
        &tcp_server_command::stall_control_settings_cmd,
    },
    {
        "TOUCH_PROBE_SETTINGS",
        &tcp_server_command::touch_probe_settings_cmd,
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
        "LOG_LEVEL",
        &tcp_server_command::log_level_cmd,
    },

    {
        "AXES_SETTINGS",
        &tcp_server_command::axes_settings_cmd,
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
    for (unsigned int i = 0; i < (sizeof(cmds_table) / sizeof(cmds_table[0])); i++) {
        if (!strcmp(cmd, cmds_table[i].cmd_name)) {
            // return cmds_table[i].cmd_function(pars);
            return (this->*(cmds_table[i].cmd_function))(pars);
        }
    }
    lDebug_uart_semihost(Error, "No matching command found");
    json::MyJsonDocument res;
    res.set("UNKNOWN COMMAND");
    return res;
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
    json::DeserializationError error = json::deserializeJson(rx_JSON_value, rx_buff);

    auto tx_JSON_value = json::MyJsonDocument();
    *tx_buff = NULL;
    int buff_len = 0;

    if (error) {
        lDebug_uart_semihost(Error, "Error json parse. %s", error.c_str());
    } else {
        for (json::JsonVariant command : rx_JSON_value.as<json::JsonArray>()) {
            char const *command_name = command["cmd"];
            lDebug_uart_semihost(Info, "Command Found: %s", command_name);
            auto pars = command["pars"];

            auto ans = cmd_execute(command_name, pars);
            tx_JSON_value[command_name] = ans;
        }

        buff_len = json::measureJson(tx_JSON_value); /* returns 0 on fail */
        buff_len++;
        *tx_buff = new char[buff_len];
        if (!(*tx_buff)) {
            lDebug_uart_semihost(Error, "Out Of Memory");
            buff_len = 0;
        } else {
            json::serializeJson(tx_JSON_value, *tx_buff, buff_len);
            lDebug_uart_semihost(Info, "%s", *tx_buff);
        }
    }
    return buff_len;
}
