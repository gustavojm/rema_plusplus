#pragma once

#include "websocket.h"

#include "xy_axes.h"
#include "z_axis.h"
#include "rema.h"

inline websocket_server joystick_ws_server;

const int BIG_NEGATIVE_NUMBER = -999999999;
const int BIG_POSITIVE_NUMBER = 999999999;

void joystick_message_handler(uint8_t *data, uint32_t len, websocket_msg_type type);