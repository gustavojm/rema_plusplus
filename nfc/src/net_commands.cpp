#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"
#include "FreeRTOS.h"
    int len;


#include "mot_pap.h"
#include "net_commands.h"
#include "json_wp.h"
#include "settings.h"
#include "rema.h"
#include "bresenham.h"
#include "z_axis.h"
#include "xy_axes.h"
#include "temperature_ds18b20.h"

#define PROTOCOL_VERSION  	"JSON_1.0"

extern mot_pap x_axis, y_axis, z_axis;

typedef struct {
    const char *cmd_name;
    JSON_Value* (*cmd_function)(JSON_Value const *pars);
} cmd_entry;

static bresenham* get_axes(const char *axis) {

    switch (*axis) {
    case 'z':
    case 'Z':
        return &z_dummy_axes_get_instance();
        break;
    default:
        return &x_y_axes_get_instance();
        break;
    }
}

static JSON_Value* logs_cmd(JSON_Value const *pars) {
    if (pars && json_value_get_type(pars) == JSONObject) {
        JSON_Object *pars_object = json_value_get_object(pars);
        double quantity = json_object_get_number(pars_object, "quantity");

        JSON_Value *root_value = json_value_init_object();
        JSON_Value *msg_array = json_value_init_array();

        int msgs_waiting = uxQueueMessagesWaiting(debug_queue);
        int extract = MIN(quantity, msgs_waiting);

        for (int x = 0; x < extract; x++) {
            char *dbg_msg = NULL;
            if (xQueueReceive(debug_queue, &dbg_msg,
                    (TickType_t) 0) == pdPASS) {
                json_array_append_string(json_value_get_array(msg_array),
                        dbg_msg);
                vPortFree(dbg_msg);
                dbg_msg = NULL;
            }
        }

        json_object_set_value(json_value_get_object(root_value), "DEBUG_MSGS",
                msg_array);
        return root_value;
    }
    return NULL;
}

static JSON_Value* protocol_version_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    json_object_set_string(json_value_get_object(root_value), "Version", PROTOCOL_VERSION);
    return root_value;
}

static JSON_Value* control_enable_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *pars_object = json_value_get_object(pars);

    if (json_object_has_value_of_type(pars_object, "enabled", JSONBoolean)) {
        bool enabled = json_object_get_boolean(pars_object, "enabled");
        rema::control_enabled_set(enabled);
    }
    json_object_set_boolean(json_value_get_object(root_value), "STATUS",
            rema::control_enabled_get());
    return root_value;
}

static JSON_Value* stall_control_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *pars_object = json_value_get_object(pars);
    if (json_object_has_value_of_type(pars_object, "enabled",
            JSONBoolean)) {
        bool enabled = json_object_get_boolean(pars_object,
                "enabled");
        rema::stall_control_set(enabled);
    }
    json_object_set_boolean(json_value_get_object(root_value), "STATUS",
            rema::stall_control_get());
    return root_value;
}

static JSON_Value* set_coords_cmd(JSON_Value const *pars) {
    if (pars && json_value_get_type(pars) == JSONObject) {
        JSON_Object *pars_object = json_value_get_object(pars);
        if (json_object_has_value(pars_object, "position_x")) {
            double pos_x = json_object_get_number(pars_object, "position_x");
            x_axis.set_position(pos_x);
        }

        if (json_object_has_value(pars_object, "position_y")) {
            double pos_y = json_object_get_number(pars_object, "position_y");
            y_axis.set_position(pos_y);
        }

        if (json_object_has_value(pars_object, "position_z")) {
            double pos_z = json_object_get_number(pars_object, "position_z");
            z_axis.set_position(pos_z);
        }
    }
    JSON_Value *root_value = json_value_init_object();
    json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
    return root_value;
}

static JSON_Value* kp_set_tunings_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    JSON_Object *pars_object = json_value_get_object(pars);

    if (pars && json_value_get_type(pars) == JSONObject) {

        char const *axes = json_object_get_string(pars_object, "axes");
        float kp = (float) json_object_get_number(pars_object, "kp");
        int update = (int) json_object_get_number(pars_object, "update");
        int min = (int) json_object_get_number(pars_object, "min");
        int max = (int) json_object_get_number(pars_object, "max");

        bresenham *axes_ = get_axes(axes);

        if (axes_ == nullptr) {
            json_object_set_boolean(root_object, "ACK", false);
            json_object_set_string(root_object, "ERROR",
                    "No axis specified");
        } else {
            axes_->step_time = std::chrono::milliseconds(update);
            axes_->kp.set_output_limits(min, max);
            axes_->kp.set_sample_period(axes_->step_time);
            axes_->kp.set_tunings(kp);
            lDebug(Debug, "KP Settings set");
            json_object_set_boolean(root_object, "ACK", true);
        }
    }
    return root_value;
}

static JSON_Value* axes_hard_stop_all_cmd(JSON_Value const *pars) {
    struct bresenham_msg *msg_xy = (struct bresenham_msg*) pvPortMalloc(
            sizeof(struct bresenham_msg));
    msg_xy->type = mot_pap::TYPE_HARD_STOP;
    if (xQueueSend((&x_y_axes_get_instance())->queue, &msg_xy, portMAX_DELAY) == pdPASS) {
        lDebug(Debug, "Command sent");
    }

    struct bresenham_msg *msg_z = (struct bresenham_msg*) pvPortMalloc(
            sizeof(struct bresenham_msg));
    msg_z->type = mot_pap::TYPE_HARD_STOP;
    if (xQueueSend((&x_y_axes_get_instance())->queue, &msg_z, portMAX_DELAY) == pdPASS) {
            lDebug(Debug, "Command sent");
    }
    JSON_Value *root_value = json_value_init_object();
    json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
    return root_value;
}

static JSON_Value* axes_soft_stop_all_cmd(JSON_Value const *pars) {
    struct bresenham_msg *msg_xy = (struct bresenham_msg*) pvPortMalloc(
            sizeof(struct bresenham_msg));
    msg_xy->type = mot_pap::TYPE_SOFT_STOP;
    if (xQueueSend((&x_y_axes_get_instance())->queue, &msg_xy, portMAX_DELAY) == pdPASS) {
        lDebug(Debug, "Command sent");
    }

    struct bresenham_msg *msg_z = (struct bresenham_msg*) pvPortMalloc(
            sizeof(struct bresenham_msg));
    msg_z->type = mot_pap::TYPE_SOFT_STOP;
    if (xQueueSend((&x_y_axes_get_instance())->queue, &msg_z, portMAX_DELAY) == pdPASS) {
            lDebug(Debug, "Command sent");
    }

    JSON_Value *root_value = json_value_init_object();
    json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
    return root_value;
}

static JSON_Value* network_settings_cmd(JSON_Value const *pars) {
    if (pars && json_value_get_type(pars) == JSONObject) {
        JSON_Object *pars_object = json_value_get_object(pars);
        char const *gw = json_object_get_string(pars_object, "gw");
        char const *ipaddr = json_object_get_string(pars_object, "ipaddr");
        char const *netmask = json_object_get_string(pars_object, "netmask");
        uint16_t port = static_cast<uint16_t>(json_object_get_number(pars_object, "port"));

        if (gw && ipaddr && netmask && port != 0) {
            lDebug(Info,
                    "Received network settings: gw:%s, ipaddr:%s, netmask:%s, port:%d",
                    gw, ipaddr, netmask, port);

            unsigned char *gw_bytes =
                    reinterpret_cast<unsigned char*>(&(settings::network.gw.addr));
            if (sscanf(gw, "%hhu.%hhu.%hhu.%hhu", &gw_bytes[0], &gw_bytes[1],
                    &gw_bytes[2], &gw_bytes[3]) == 4) {
            }

            unsigned char *ipaddr_bytes =
                    reinterpret_cast<unsigned char*>(&(settings::network.ipaddr.addr));
            if (sscanf(ipaddr, "%hhu.%hhu.%hhu.%hhu", &ipaddr_bytes[0],
                    &ipaddr_bytes[1], &ipaddr_bytes[2], &ipaddr_bytes[3])
                    == 4) {
            }

            unsigned char *netmask_bytes =
                    reinterpret_cast<unsigned char*>(&(settings::network.netmask.addr));
            if (sscanf(netmask, "%hhu.%hhu.%hhu.%hhu", &netmask_bytes[0],
                    &netmask_bytes[1], &netmask_bytes[2], &netmask_bytes[3])
                    == 4) {
            }

            settings::network.port = port;

            settings::save();
            lDebug(Info, "Settings saved. Restarting...");

            Chip_UART_SendBlocking(DEBUG_UART, "\n\n", 2);

            Chip_RGU_TriggerReset(RGU_CORE_RST);
        }

        JSON_Value *root_value = json_value_init_object();
        json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
        return root_value;
    }
    return NULL;
}

static JSON_Value* mem_info_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "MEM_TOTAL", configTOTAL_HEAP_SIZE);
    json_object_set_number(root_object, "MEM_FREE", xPortGetFreeHeapSize());
    json_object_set_number(root_object, "MEM_MIN_FREE", xPortGetMinimumEverFreeHeapSize());
    return root_value;
}

static JSON_Value* temperature_info_cmd(JSON_Value const *pars) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "TEMP X", (static_cast<double>(temperature_ds18b20_get(0))) / 10);
    json_object_set_number(root_object, "TEMP Y", (static_cast<double>(temperature_ds18b20_get(1))) / 10);
    json_object_set_number(root_object, "TEMP Z", (static_cast<double>(temperature_ds18b20_get(2))) / 10);
    return root_value;
}

static JSON_Value* move_closed_loop_cmd(JSON_Value const *pars) {
    if (pars && json_value_get_type(pars) == JSONObject) {
        JSON_Object *pars_object = json_value_get_object(pars);
        char const *axes = json_object_get_string(pars_object, "axes");
        bresenham *axes_ = get_axes(axes);

        double first_axis_setpoint = json_object_get_number(pars_object, "first_axis_setpoint");

        double second_axis_setpoint = json_object_get_number(pars_object, "second_axis_setpoint");

        struct bresenham_msg *msg = (struct bresenham_msg*) pvPortMalloc(
                sizeof(struct bresenham_msg));
        msg->type = mot_pap::TYPE_BRESENHAM;
        msg->first_axis_setpoint = static_cast<int>(first_axis_setpoint
                * axes_->first_axis->inches_to_counts_factor);
        msg->second_axis_setpoint = static_cast<int>(second_axis_setpoint
                * axes_->second_axis->inches_to_counts_factor);

        if (xQueueSend(axes_->queue, &msg, portMAX_DELAY) == pdPASS) {
            lDebug(Debug, "Command sent");
        }

        lDebug(Info, "AXIS_BRESENHAM SETPOINT X= %f, SETPOINT Y=%f",
                first_axis_setpoint, second_axis_setpoint);
    }
    JSON_Value *root_value = json_value_init_object();
    json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
    return root_value;
}

static JSON_Value* move_free_run_cmd(JSON_Value const *pars) {
    if (pars && json_value_get_type(pars) == JSONObject) {
        JSON_Object *pars_object = json_value_get_object(pars);
        char const *axes = json_object_get_string(pars_object, "axes");
        bresenham *axes_ = get_axes(axes);

        int first_axis_setpoint, second_axis_setpoint;
        if (json_object_has_value_of_type(pars_object,
                "first_axis_setpoint", JSONNumber)) {
            first_axis_setpoint = static_cast<int>(json_object_get_number(
                    pars_object, "first_axis_setpoint"));
        } else {
            first_axis_setpoint = axes_->first_axis->current_counts();
        }

        if (json_object_has_value_of_type(pars_object, "second_axis_setpoint", JSONNumber)) {
            second_axis_setpoint = static_cast<int>(json_object_get_number(
                    pars_object, "second_axis_setpoint"));
        } else {
            second_axis_setpoint = axes_->second_axis->current_counts();
        }

        struct bresenham_msg *msg = (struct bresenham_msg*) pvPortMalloc(
                sizeof(struct bresenham_msg));
        msg->type = mot_pap::TYPE_BRESENHAM;
        msg->first_axis_setpoint = first_axis_setpoint;
        msg->second_axis_setpoint = second_axis_setpoint;

        if (xQueueSend(axes_->queue, &msg, portMAX_DELAY) == pdPASS) {
            lDebug(Debug, "Command sent");
        }

        lDebug(Info, "AXIS_BRESENHAM SETPOINT X= %i, SETPOINT Y=%i",
                first_axis_setpoint, second_axis_setpoint);
    }
    JSON_Value *root_value = json_value_init_object();
    json_object_set_boolean(json_value_get_object(root_value), "ACK", true);
    return root_value;
}

// @formatter:off
const cmd_entry cmds_table[] = {
        {
                "PROTOCOL_VERSION",     /* Command name */
                protocol_version_cmd,   /* Associated function */
        },
        {
                "CONTROL_ENABLE",
                control_enable_cmd,
        },
        {
                "STALL_CONTROL",
                stall_control_cmd,
        },
        {
                "AXES_HARD_STOP_ALL",
                axes_hard_stop_all_cmd,
        },
        {
                "AXES_SOFT_STOP_ALL",
                axes_soft_stop_all_cmd,
        },
        {
                "LOGS",
                logs_cmd,
        },
        {
                "KP_SET_TUNINGS",
                kp_set_tunings_cmd,
        },
        {
                "NETWORK_SETTINGS",
                network_settings_cmd,
        },
        {
                "MEM_INFO",
                mem_info_cmd,
        },
        {
                "TEMP_INFO",
                temperature_info_cmd,
        },
        {
                "SET_COORDS",
                set_coords_cmd,
        },
        {
                "MOVE_FREE_RUN",
                move_free_run_cmd,
        },
        {
                "MOVE_CLOSED_LOOP",
                move_closed_loop_cmd,
        },

};
// @formatter:on

/**
 * @brief 	searchs for a matching command name in cmds_table[], passing the parameters
 * 			as a JSON object for the called function to parse them.
 * @param 	*cmd 	:name of the command to execute
 * @param   *pars   :JSON object containing the passed parameters to the called function
 */
JSON_Value* cmd_execute(char const *cmd, JSON_Value const *pars) {
    bool cmd_found = false;
    for (unsigned int i = 0; i < (sizeof(cmds_table) / sizeof(cmds_table[0]));
            i++) {
        if (!strcmp(cmd, cmds_table[i].cmd_name)) {
            return cmds_table[i].cmd_function(pars);
        }
    }
    if (!cmd_found) {
        lDebug(Error, "No matching command found");
    }
    return NULL;
}
