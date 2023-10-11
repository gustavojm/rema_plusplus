
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif

#include "board.h"
#include "arch/lpc18xx_43xx_emac.h"
#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h" /* For the PHY monitor support */
#include "settings.h"
#include "tcp_server_comm.h"
#include "tcp_server_telemetry.h"
#include "debug.h"

#define ip_addr_print(ipaddr) \
  printf("IP ADDRESS FROM EEPROM = %hhu.%hhu.%hhu.%hhu",         \
      ip4_addr1(ipaddr),       \
      ip4_addr2(ipaddr),       \
      ip4_addr3(ipaddr),       \
      ip4_addr4(ipaddr))

/* When building the example to run in FLASH, the number of available
 pbufs and memory size (in lwipopts.h) and the number of descriptors
 (in lpc_18xx43xx_emac_config.h) can be increased due to more available
 IRAM. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* NETIF data */
static struct netif lpc_netif;

/* Callback for TCPIP thread to indicate TCPIP init is done */
static void tcpip_init_done_signal(void *arg) {
    /* Tell main thread TCP/IP init is done */
    *reinterpret_cast<s32_t*>(arg) = 1;
}

/* LWIP kickoff and PHY link monitor thread */
extern "C" void vStackIpSetup(void *pvParameters) {
    ip_addr_t ipaddr, netmask, gw;
    volatile s32_t tcpipdone = 0;
    uint32_t physts;
    static int prt_ip = 0;

    /* Wait until the TCP/IP thread is finished before
     continuing or wierd things may happen */
    LWIP_DEBUGF(LWIP_DBG_ON, ("Waiting for TCPIP thread to initialize...\n"));
    tcpip_init(tcpip_init_done_signal, const_cast<void*>(reinterpret_cast<volatile void*>(&tcpipdone)));
    while (!tcpipdone) {
        msDelay(1);
    }

    LWIP_DEBUGF(LWIP_DBG_ON, ("Starting LWIP TCP server...\n"));

    /* Static IP assignment */
#if LWIP_DHCP
    IP4_ADDR(&gw, 0, 0, 0, 0);
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
    settings::read();

    ipaddr = settings::network.ipaddr;
    gw = settings::network.gw;
    netmask = settings::network.netmask;
#endif

    /* Add netif interface  */
    if (!netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
            tcpip_input)) {
        LWIP_ASSERT("Net interface failed to initialize\r\n", 0);
    }
    netif_set_default(&lpc_netif);
    netif_set_up(&lpc_netif);

    /* Enable MAC interrupts only after LWIP is ready */
    NVIC_SetPriority(ETHERNET_IRQn, config_ETHERNET_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(ETHERNET_IRQn);

#if LWIP_DHCP
    dhcp_start(&lpc_netif);
#endif

    /* Initialize and start application */
    stackIp_ThreadInit(settings::network.port);
    stackIp_Telemetry_ThreadInit(settings::network.port + 1);

    /* This loop monitors the PHY link and will handle cable events
     via the PHY driver. */
    while (1) {
        /* Call the PHY status update state machine once in a while
         to keep the link status up-to-date */
        physts = lpcPHYStsPoll();

        /* Only check for connection state when the PHY status has changed */
        if (physts & PHY_LINK_CHANGED) {
            if (physts & PHY_LINK_CONNECTED) {
                prt_ip = 0;

                /* Set interface speed and duplex */
                if (physts & PHY_LINK_SPEED100) {
                    Chip_ENET_SetSpeed(LPC_ETHERNET, 1);
                    NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 100000000);
                } else {
                    Chip_ENET_SetSpeed(LPC_ETHERNET, 0);
                    NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 10000000);
                }
                if (physts & PHY_LINK_FULLDUPLX) {
                    Chip_ENET_SetDuplex(LPC_ETHERNET, true);
                } else {
                    Chip_ENET_SetDuplex(LPC_ETHERNET, false);
                }

                tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_up,
                        reinterpret_cast<void*>(&lpc_netif), 1);
            } else {
                tcpip_callback_with_block(
                        (tcpip_callback_fn) netif_set_link_down,
                        reinterpret_cast<void*>(&lpc_netif), 1);
            }

            DEBUGOUT("Link connect status: %d\r\n",
                    ((physts & PHY_LINK_CONNECTED) != 0));

            /* Delay for link detection (250mS) */
            vTaskDelay(configTICK_RATE_HZ / 4);
        }

        /* Print IP address info */
        if (!prt_ip) {
            if (lpc_netif.ip_addr.addr) {
                static char tmp_buff[16];
                DEBUGOUT(
                        "\n\n\r - NASA GSPC - \n REMA Remote Terminal Unit. \n Attempting to open interface.\n\n\r");
                DEBUGOUT("IP_ADDR    : %s\r\n",
                        ipaddr_ntoa_r((const ip_addr_t*) &lpc_netif.ip_addr,
                                tmp_buff, 16));
                DEBUGOUT("NET_MASK   : %s\r\n",
                        ipaddr_ntoa_r((const ip_addr_t*) &lpc_netif.netmask,
                                tmp_buff, 16));
                DEBUGOUT("GATEWAY_IP : %s\r\n",
                        ipaddr_ntoa_r((const ip_addr_t*) &lpc_netif.gw,
                                tmp_buff, 16));
                prt_ip = 1;
            }
        }
    }
}
