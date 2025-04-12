#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "debug.h"

#include "lwip/api.h"

#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "crypto.h"

#define WS_PORT 8765
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_FIN_FLAG                1 << 7
#define WS_MASKED_FLAG             1 << 7
#define WS_TYPE_MASK               0xF
#define WS_TYPE_TEXT 0x01
#define WS_TYPE_BINARY 0x02
#define WS_TYPE_CLOSE 0x08
#define WS_TYPE_PING 0x09
#define WS_TYPE_PONG 0x0A
#define SELECT_TIMEOUT_MS 100

#define WS_MAX_CLIENTS             1
#define WS_SEND_BUFFER_SIZE        512
#define WS_RECV_BUFFER_SIZE        512

#define WS_MAX_PAYLOAD_LENGTH 512

inline QueueHandle_t websocketQueue;

typedef enum {
    WS_TYPE_STRING = WS_TYPE_TEXT,
    WS_TYPE_BIN = WS_TYPE_BINARY
} ws_type_t;

typedef struct {
    uint8_t *message;
    uint32_t msg_size;
    ws_type_t msg_type;
} ws_msg_t;

typedef struct {
    char payload[WS_SEND_BUFFER_SIZE];
    uint32_t payload_length;
} WebsocketPublishMessage;

// Forward declaration
typedef struct ws_server_s ws_server_t;

typedef void (*ws_callback_t)(uint8_t *payload, uint32_t length, ws_type_t type);

typedef struct {
    int socket;
    bool established;
    uint8_t recv_buf[WS_RECV_BUFFER_SIZE];
    ws_server_t *server_ptr;
} ws_client_t;

struct ws_server_s {
    uint8_t send_buf[WS_SEND_BUFFER_SIZE];
    ws_client_t client;
    ws_callback_t msg_handler;
};

void ws_server_init(ws_server_t *ws, ws_callback_t callback);
void ws_send_message(ws_server_t *ws, ws_msg_t *msg);
int sendToWebsocketQueue(const char *payload);