/* BSD Socket API Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
//#include <string.h>
#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "json_wp.h"
#include "tcp_server_command.h"
#include "debug.h"
#include "xy_axes.h"
#include "z_axis.h"
#include "rema.h"

static void stop_all() {
    x_y_axes_get_instance().stop();
    z_dummy_axes_get_instance().stop();
    lDebug(Warn, "Stopping all");
}

void do_retransmit(const int sock) {
    int len;
    char rx_buffer[1024];

    do {
        len = lwip_recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            stop_all();
            lDebug(Error, "Error occurred during receiving command: errno %d", errno);
        } else if (len == 0) {
            stop_all();
            lDebug(Warn, "Command connection closed");
        } else {
            //rema::update_watchdog_timer();
            rx_buffer[len] = 0;  // Null-terminate whatever is received and treat it like a string
            lDebug(InfoLocal, "Command received %s", rx_buffer);

            char *tx_buffer;

            int ack_len = json_wp(rx_buffer, &tx_buffer);

            // lDebug(InfoLocal, "To send %d bytes: %s", ack_len, tx_buffer);

            if (ack_len > 0) {
                // send() can return less bytes than supplied length.
                // Walk-around for robust implementation.
                int to_write = ack_len;

                while (to_write > 0) {
                    int written = lwip_send(sock, tx_buffer + (ack_len - to_write),
                            to_write, 0);
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


