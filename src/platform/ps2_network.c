/*
 * File synopsis:
 * Owns qualified PS2 Ethernet startup, fixed private-link configuration, and
 * creation/closure of caller-owned TCP descriptors for fixed product endpoints.
 * Physical PSTV I/O after Transport adoption remains exclusively Transport-owned;
 * separate management descriptors never participate in Transport adoption.
 *
 * Context: docs/reconstruction/ISSUE7_MINIMAL_CORE.md, "PS2 system and
 * private-Ethernet platform seam"; docs/ledge/LEDGE_ARCHITECTURE_OVERLAY.md;
 * docs/ledge/LEDGE_FOREMAN_STATE.md, R31.
 */

#include <kernel.h>
#include <loadfile.h>
#include <netman.h>
#include <ps2ip.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string.h>

#include "ps2_network.h"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;
extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;
extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

static void link_wait_alarm(s32 alarm_id, u16 time, void *common)
{
    (void)alarm_id;
    (void)time;
    iWakeupThread(*(int *)common);
}

static int link_is_up(void)
{
    return NetManIoctl(
        NETMAN_NETIF_IOCTL_GET_LINK_STATUS,
        NULL,
        0,
        NULL,
        0) == NETMAN_NETIF_ETH_LINK_STATE_UP;
}

static int ps2_network_connect_server(
    const char *server_ip,
    unsigned short server_port)
{
    int socket_fd;
    struct sockaddr_in server;

    if (server_ip == NULL || server_port == 0u)
        return -1;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
        return -1;

    memset(&server, 0, sizeof(server));
    server.sin_len = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(
            socket_fd,
            (struct sockaddr *)&server,
            sizeof(server)) < 0) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

int pstvnc_ps2_network_init(void)
{
    struct ip4_addr local_ip;
    struct ip4_addr netmask;
    struct ip4_addr gateway;

    if (SifExecModuleBuffer(
            DEV9_irx,
            size_DEV9_irx,
            0,
            NULL,
            NULL) < 0)
        return -1;

    if (SifExecModuleBuffer(
            NETMAN_irx,
            size_NETMAN_irx,
            0,
            NULL,
            NULL) < 0)
        return -1;

    if (SifExecModuleBuffer(
            SMAP_irx,
            size_SMAP_irx,
            0,
            NULL,
            NULL) < 0)
        return -1;

    if (NetManInit() < 0)
        return -1;

    IP4_ADDR(&local_ip, 192, 168, 50, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gateway, 192, 168, 50, 1);

    if (ps2ipInit(&local_ip, &netmask, &gateway) < 0)
        return -1;

    return 0;
}

int pstvnc_ps2_network_wait_link(void)
{
    int thread_id = GetThreadId();
    int retry_cycles;

    for (retry_cycles = 0; !link_is_up(); retry_cycles++) {
        if (SetAlarm(1000 * 16, &link_wait_alarm, &thread_id) < 0)
            return -1;

        SleepThread();

        if (retry_cycles >= 9)
            return -1;
    }

    return 0;
}

int pstvnc_ps2_network_connect_pstv(void)
{
    return ps2_network_connect_server(
        PSTVNC_PS2_PSTV_SERVER_IP,
        PSTVNC_PS2_PSTV_SERVER_PORT);
}

int pstvnc_ps2_network_connect_management(void)
{
    return ps2_network_connect_server(
        PSTVNC_PS2_MANAGEMENT_SERVER_IP,
        PSTVNC_PS2_MANAGEMENT_SERVER_PORT);
}

void pstvnc_ps2_network_close(int socket_fd)
{
    if (socket_fd >= 0)
        close(socket_fd);
}
