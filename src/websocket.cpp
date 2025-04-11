#include "websocket.h"


const char *head_ws = "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: ";

static char *get_ws_key(char *buf, size_t *len) {
    char *p = strstr(buf, "Sec-WebSocket-Key: ");
    if (p) {
        p = p + strlen("Sec-WebSocket-Key: ");
        *len = strchr(p, '\r') - p;
    }
    return p;
}

static char *create_ws_key_accept(char *inbuf) {
    static char concat_key[64] = {0};
    static char hash[22] = {0};
    static char hash_base64[64] = {0};
    size_t len = 0;

    char *key = get_ws_key(inbuf, &len);
    if (!key) return NULL;

    memset(concat_key, 0, sizeof(concat_key));
    strncpy(concat_key, key, len);
    strcat(concat_key, WS_GUID);
    
    sha1_ctx_t ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t *)concat_key, 60);
    sha1_final(&ctx, (uint8_t *)hash);

    base64_encode((uint8_t *)hash, 20, hash_base64);
    return hash_base64;
}

static uint32_t get_message_len(uint8_t *msg) {
    uint32_t len = 0;

    if ((msg[1] & 0x7F) == 126) {
        len = (msg[2] << 8) | msg[3];
    } else if ((msg[1] & 0x7F) == 127) {
        // 64-bit length not supported in this implementation
        len = 0;
    } else {
        len = msg[1] & 0x7F;
    }

    return len;
}

static bool is_masked_msg(uint8_t *msg) {
    return (msg[1] & WS_MASKED_FLAG);
}

static bool is_fin_msg(uint8_t *msg) {
    return (msg[0] & WS_FIN_FLAG);
}

static uint8_t *get_mask(uint8_t *msg) {
    if ((msg[1] & 0x7F) == 126) {
        return &msg[4];
    } else if ((msg[1] & 0x7F) == 127) {
        return &msg[10];  // 64-bit length not fully supported
    } else {
        return &msg[2];
    }
}

static uint8_t *get_payload_ptr(uint8_t *msg) {
    uint8_t *p;
    
    if ((msg[1] & 0x7F) == 126) {
        p = msg + 4;
    } else if ((msg[1] & 0x7F) == 127) {
        p = msg + 10;  // 64-bit length not fully supported
    } else {
        p = msg + 2;
    }
    
    if (is_masked_msg(msg)) {
        p += 4;
    }
    
    return p;
}

static void unmask_message_payload(uint8_t *pld, uint32_t len, uint8_t *mask) {
    for (int i = 0; i < len; i++) {
        pld[i] = mask[i % 4] ^ pld[i];
    }
}

static uint8_t *ws_set_size_to_frame(uint32_t size, uint8_t *out_frame) {
    uint8_t *out_frame_ptr = out_frame;
    
    if (size < 126) {
        *out_frame_ptr = size;
        out_frame_ptr++;
    } else if (size < 65536) {  // 16-bit length
        out_frame_ptr[0] = 126;
        out_frame_ptr[1] = ((size >> 8) & 0xFF);
        out_frame_ptr[2] = (size & 0xFF);
        out_frame_ptr += 3;
    } else {  // 64-bit length - not fully implemented
        out_frame_ptr[0] = 127;
        memset(&out_frame_ptr[1], 0, 6); // Clear first 6 bytes (most significant)
        out_frame_ptr[7] = ((size >> 8) & 0xFF);
        out_frame_ptr[8] = (size & 0xFF);
        out_frame_ptr += 9;
    }
    
    return out_frame_ptr;
}

static uint8_t *ws_set_data_to_frame(uint8_t *data, uint32_t size, uint8_t *out_frame) {
    memcpy(out_frame, data, size);
    return (out_frame + size);
}

void ws_send_message(ws_server_t *ws, ws_msg_t *msg) {
    uint8_t *outbuf_ptr = ws->send_buf;
    ws_client_t *client = &ws->client;

    if (msg->msg_size + 10 > WS_SEND_BUFFER_SIZE) {
        lDebug(Info, "Message too large for buffer");
        return;
    }

    if (!client->established) {
        lDebug(Info, "No client connected");
        return;
    }

    memset(outbuf_ptr, 0x00, WS_SEND_BUFFER_SIZE);
    outbuf_ptr[0] = (uint8_t)msg->msg_type | WS_FIN_FLAG;
    outbuf_ptr = ws_set_size_to_frame(msg->msg_size, &outbuf_ptr[1]);
    outbuf_ptr = ws_set_data_to_frame(msg->message, msg->msg_size, outbuf_ptr);
    size_t packet_size = outbuf_ptr - ws->send_buf;

    // Set send timeout using setsockopt
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // 500 ms
    lwip_setsockopt(client->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    int bytes_sent = lwip_send(client->socket, ws->send_buf, packet_size, 0);
    if (bytes_sent < 0) {
        lDebug(Error, "Write failed with err %d (\"%s\")", errno, strerror(errno));
    } else {
        lDebug(Info, "Sent %d bytes to client", bytes_sent);
    }
}

static void handle_websocket_frame(ws_server_t *ws, uint8_t *buffer, int length) {
    if (length < 2) return;
    
    uint8_t *inbuf_ptr = buffer;
    
    // Check if it's a control frame
    if ((inbuf_ptr[0] & WS_TYPE_MASK) == WS_TYPE_PING) {
        lDebug(Info, "Received PING, sending PONG");
        // Send PONG response
        uint8_t pong_frame[2] = {WS_FIN_FLAG | WS_TYPE_PONG, 0};
        lwip_send(ws->client.socket, pong_frame, 2, 0);
        return;
    }
    
    if ((inbuf_ptr[0] & WS_TYPE_MASK) == WS_TYPE_PONG) {
        lDebug(Info, "Received PONG");
        return;
    }
    
    if ((inbuf_ptr[0] & WS_TYPE_MASK) == WS_TYPE_CLOSE) {
        lDebug(Info, "Received CLOSE frame");
        // Echo close frame
        uint8_t close_frame[2] = {WS_FIN_FLAG | WS_TYPE_CLOSE, 0};
        lwip_send(ws->client.socket, close_frame, 2, 0);
        return;
    }
    
    // Handle data frame
    if (is_fin_msg(inbuf_ptr)) {
        uint32_t len = get_message_len(inbuf_ptr);
        uint8_t *payload = get_payload_ptr(inbuf_ptr);
        
        if (is_masked_msg(inbuf_ptr)) {
            uint8_t *mask = get_mask(inbuf_ptr);
            unmask_message_payload(payload, len, mask);
        }
        
        if (ws->msg_handler) {
            ws->msg_handler(payload, len, (ws_type_t)(inbuf_ptr[0] & WS_TYPE_MASK));
        }
    }
}

static bool process_websocket_handshake(ws_server_t *ws, uint8_t *buffer) {
    if (strncmp((char *)buffer, "GET /", 5) != 0) {
        return false;
    }
    
    char *ws_key_accept = create_ws_key_accept((char *)buffer);
    if (!ws_key_accept) {
        lDebug(Error, "Invalid WebSocket handshake request");
        return false;
    }
    
    // Create handshake response
    int response_len = snprintf((char *)ws->send_buf, WS_SEND_BUFFER_SIZE,
                               "%s%s\r\n\r\n", head_ws, ws_key_accept);
    
    if (lwip_send(ws->client.socket, ws->send_buf, response_len, 0) < 0) {
        lDebug(Error, "Failed to send handshake response");
        return false;
    }
    
    lDebug(Info, "WebSocket handshake completed");
    return true;
}

void ws_server_task(void *arg) {
    websocketQueue = xQueueCreate(10, sizeof(WebsocketPublishMessage));
    
    ws_server_t *ws = (ws_server_t *)arg;
    fd_set read_fds;
    struct timeval timeout;
    WebsocketPublishMessage queued_msg;
    
    // Create listening socket
    int listen_sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        lDebug(Error, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }
    
    // Set socket options for reuse
    int opt = 1;
    lwip_setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Prepare server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(WS_PORT);
    
    // Bind socket
    if (lwip_bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        lDebug(Error, "Socket bind failed");
        lwip_close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    
    // Listen for connections
    if (lwip_listen(listen_sock, 1) < 0) {
        lDebug(Error, "Listen failed");
        lwip_close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    
    lDebug(Info, "WebSocket server started on port %d", WS_PORT);
    
    // Initialize the client structure
    memset(&ws->client, 0, sizeof(ws_client_t));
    ws->client.socket = -1;
    ws->client.established = false;
    ws->client.server_ptr = ws;
    
    while (1) {
        // Check if we need to accept a new client
        if (ws->client.socket < 0) {
            // lDebug(Info, "Waiting for WebSocket client...");
            
            // Set up for select on listen socket
            FD_ZERO(&read_fds);
            FD_SET(listen_sock, &read_fds);
            timeout.tv_sec = 0;
            timeout.tv_usec = SELECT_TIMEOUT_MS * 1000;
            
            if (lwip_select(listen_sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_sock = lwip_accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
                
                if (client_sock >= 0) {
                    ws->client.socket = client_sock;
                    lDebug(Info, "New client connected");
                }
            }
        } else {
            // We have a client connected
            FD_ZERO(&read_fds);
            FD_SET(ws->client.socket, &read_fds);
            timeout.tv_sec = 0;
            timeout.tv_usec = SELECT_TIMEOUT_MS * 1000;
            
            int select_result = lwip_select(ws->client.socket + 1, &read_fds, NULL, NULL, &timeout);
            
            if (select_result > 0 && FD_ISSET(ws->client.socket, &read_fds)) {
                // Data available from client
                memset(ws->client.recv_buf, 0, WS_RECV_BUFFER_SIZE);
                int recv_bytes = lwip_recv(ws->client.socket, ws->client.recv_buf, WS_RECV_BUFFER_SIZE, 0);
                
                if (recv_bytes <= 0) {
                    // Client disconnected
                    lDebug(Info, "Client disconnected");
                    lwip_close(ws->client.socket);
                    ws->client.socket = -1;
                    ws->client.established = false;
                    continue;
                }
                
                // Process received data
                if (!ws->client.established) {
                    // Handle WebSocket handshake
                    if (process_websocket_handshake(ws, ws->client.recv_buf)) {
                        ws->client.established = true;
                    } else {
                        // Invalid handshake
                        lDebug(Error, "Invalid WebSocket handshake");
                        lwip_close(ws->client.socket);
                        ws->client.socket = -1;
                    }
                } else {
                    // Handle WebSocket frame
                    handle_websocket_frame(ws, ws->client.recv_buf, recv_bytes);
                }
            } else if (select_result < 0) {
                // Error in select
                lDebug(Error, "Error in select(). Closing client");
                lwip_close(ws->client.socket);
                ws->client.socket = -1;
                ws->client.established = false;
            }
        }
        
        // Check for queued messages to send
        if (ws->client.established && xQueueReceive(websocketQueue, &queued_msg, 0) == pdPASS) {
            ws_msg_t ws_msg;
            ws_msg.message = (uint8_t *)&queued_msg.payload;
            ws_msg.msg_size = queued_msg.payload_length;
            ws_msg.msg_type = WS_TYPE_STRING;
            ws_send_message(ws, &ws_msg);
            lDebug(Info, "Sent queued message to client");
        }
    }
}

int sendToWebsocketQueue(const char *payload) {
    WebsocketPublishMessage msg;
    strncpy(msg.payload, payload, sizeof msg.payload);
    msg.payload_length = strlen(payload);
    
    if (xQueueSend(websocketQueue, &msg, 0) != pdPASS) {
        lDebug(Warn, "WebsocketQueue is full");
        return -1;
    }

    return 0;
}

void ws_server_init(ws_server_t *ws, ws_callback_t callback) {
    ws->msg_handler = callback;
    
    TaskHandle_t ws_serverTask_handle;
    xTaskCreate(ws_server_task, "ws_server", configMINIMAL_STACK_SIZE * 4, 
                (void *)ws, (configMAX_PRIORITIES - 1), &ws_serverTask_handle);
}
