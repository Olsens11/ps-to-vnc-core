/*
 * Hardware checkpoint 01: PS2 platform + private Ethernet + TCP round trip.
 * Only the imported platform/system and platform/network seams are exercised.
 */

#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

#include "platform/ps2_network.h"
#include "platform/ps2_system.h"

#define CHECKPOINT_HELLO "PSTVNC_CORE_PLATFORM_NETWORK_HELLO\n"
#define CHECKPOINT_ACK   "PSTVNC_CORE_PLATFORM_NETWORK_ACK\n"
#define CHECKPOINT_PASS  "PSTVNC_CORE_PLATFORM_NETWORK_PASS\n"

static int send_all(int socket_fd, const char *data, size_t length)
{
    size_t sent = 0u;
    while (sent < length) {
        int result = send(socket_fd, data + sent, length - sent, 0);
        if (result <= 0)
            return 0;
        sent += (size_t)result;
    }
    return 1;
}

static int receive_exact(int socket_fd, char *data, size_t length)
{
    size_t received = 0u;
    while (received < length) {
        int result = recv(socket_fd, data + received, length - received, 0);
        if (result <= 0)
            return 0;
        received += (size_t)result;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char hello[] = CHECKPOINT_HELLO;
    static const char expected_ack[] = CHECKPOINT_ACK;
    static const char pass[] = CHECKPOINT_PASS;
    char ack[sizeof(expected_ack) - 1u];
    int socket_fd = -1;
    int success = 0;

    (void)argc;
    (void)argv;

    if (pstvnc_ps2_system_prepare_iop() < 0)
        goto done;
    if (pstvnc_ps2_network_init() < 0)
        goto done;
    if (pstvnc_ps2_network_wait_link() < 0)
        goto done;

    socket_fd = pstvnc_ps2_network_connect_management();
    if (socket_fd < 0)
        goto done;
    if (!send_all(socket_fd, hello, sizeof(hello) - 1u))
        goto done;
    if (!receive_exact(socket_fd, ack, sizeof(ack)))
        goto done;
    if (memcmp(ack, expected_ack, sizeof(ack)) != 0)
        goto done;
    if (!send_all(socket_fd, pass, sizeof(pass) - 1u))
        goto done;

    success = 1;

done:
    if (socket_fd >= 0)
        pstvnc_ps2_network_close(socket_fd);
    if (!success)
        return 1;
    pstvnc_ps2_system_exit_to_menu();
    return 0;
}
