#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "tcp_server.h"
#include "debug.h"
#include "bresenham.h"
#include "z_axis.h"
#include "rema.h"
#include "temperature_ds18b20.h"

#define KEEPALIVE_IDLE              (5)
#define KEEPALIVE_INTERVAL          (5)
#define KEEPALIVE_COUNT             (3)

tcp_server::tcp_server(const char *name, int port) :
        name(name), port(port) {

    char task_name[configMAX_TASK_NAME_LEN];
    memset(task_name, 0, sizeof(task_name));
    strncat(task_name, name, sizeof(task_name) - strlen(task_name) - 1);
    strncat(task_name, "_task", sizeof(task_name) - strlen(task_name) - 1);
    xTaskCreate([](void* me) { static_cast<tcp_server*>(me)->task();}, task_name, 1024, this,
    configMAX_PRIORITIES, NULL);

    lDebug(Info, "%s: created", task_name);

}

void tcp_server::task() {
    char addr_str[128];
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_in dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in*) &dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(port);
    ip_protocol = IPPROTO_IP;

    int listen_sock = lwip_socket(AF_INET, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        lDebug(Error, "Unable to create %s socket: errno %d", name, errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    lwip_setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    lDebug(Info, "%s socket created", name);

    int err = lwip_bind(listen_sock, (struct sockaddr*) &dest_addr,
            sizeof(dest_addr));
    if (err != 0) {
        lDebug(Error, "%s socket unable to bind: errno %d", name, errno);
        lDebug(Error, "IPPROTO: %d", AF_INET);
        goto CLEAN_UP;
    }
    lDebug(Info, "%s socket bound, port %d", name, port);

    err = lwip_listen(listen_sock, 1);
    if (err != 0) {
        lDebug(Error, "Error occurred during %s listen: errno %d", name, errno);
        goto CLEAN_UP;
    }

    while (1) {
        lDebug(Info, "%s socket listening", name);

        struct sockaddr source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = lwip_accept(listen_sock, (struct sockaddr*) &source_addr,
                &addr_len);
        if (sock < 0) {
            lDebug(Error, "Unable to accept %s connection: errno %d", name, errno);
            break;
        }

        // Set tcp keepalive option
        lwip_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive,
                sizeof(int));
        lwip_setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle,
                sizeof(int));
        lwip_setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval,
                sizeof(int));
        lwip_setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount,
                sizeof(int));
        // Convert ip address to string
        if (source_addr.sa_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in* )&source_addr)->sin_addr,
                    addr_str, sizeof(addr_str) - 1);
        }
        // lDebug(Info, "%s socket accepted ip address: %s", name, addr_str);

        reply_fn(sock);

        lwip_shutdown(sock, 0);
        lwip_close(sock);
    }

    CLEAN_UP: lwip_close(listen_sock);
    vTaskDelete(NULL);
}

