#include "joystick.h"

void joystick_message_handler(uint8_t *data, uint32_t len, websocket_msg_type type) {
    lDebug(Debug, "Joystick received: %.*s", len, data);

    bresenham_msg msg = {};
    bresenham *axes_ = nullptr;

    char first_axis_dir = data[1];
    switch (first_axis_dir) {

    case '_':
        axes_ = x_y_axes;
        msg.first_axis_setpoint = x_y_axes->first_axis->current_counts;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'L':
        axes_ = x_y_axes;
        msg.first_axis_setpoint = BIG_NEGATIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'R':
        axes_ = x_y_axes;
        msg.first_axis_setpoint = BIG_POSITIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'I':
        axes_ = z_dummy_axes;
        msg.first_axis_setpoint = BIG_POSITIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'O':
        axes_ = z_dummy_axes;
        msg.first_axis_setpoint = BIG_NEGATIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'S':
    default: 
        x_y_axes->empty_queue();
        x_y_axes->send({ mot_pap::SOFT_STOP });

        z_dummy_axes->empty_queue();
        z_dummy_axes->send({ mot_pap::SOFT_STOP });
        axes_ = nullptr;
        break;
    }

    char second_axis_dir = data[2];
    switch (second_axis_dir) {

    case '_':
        msg.second_axis_setpoint = x_y_axes->second_axis->current_counts;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'U':
        axes_ = x_y_axes;
        msg.second_axis_setpoint = BIG_POSITIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'D':
        axes_ = x_y_axes;
        msg.second_axis_setpoint = BIG_NEGATIVE_NUMBER;
        msg.type = mot_pap::type::MOVE;
        break;

    case 'S':
    default: 
        // No need to send stop as it was sent by previous S
        axes_ = nullptr;
        break;
    }

    if (axes_) {
        auto check_result = rema::check_control_and_brakes(axes_);
        if (!check_result) {
            joystick_ws_server.send_to_queue(check_result.error());
        } else {
            axes_->send(msg);
        }                    
    }
}