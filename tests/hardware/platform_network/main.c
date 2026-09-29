/*
 * Hardware checkpoint 01 stability qualification.
 * Exercises only imported platform/system and platform/network seams.
 */
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "platform/ps2_network.h"
#include "platform/ps2_system.h"

#define MAGIC 0x50535456u
#define TYPE_PING 1u
#define TYPE_IDLE_READY 2u
#define TYPE_WAKE 3u
#define TYPE_WOKE 4u
#define TYPE_DONE 5u
#define PHASE_ONE 1u
#define PHASE_TWO 2u
#define ROUND_COUNT 128u

struct checkpoint_record {
    uint32_t magic;
    uint32_t type;
    uint32_t phase;
    uint32_t sequence;
};

static void make_record(
    struct checkpoint_record *record,
    uint32_t type,
    uint32_t phase,
    uint32_t sequence)
{
    record->magic = htonl(MAGIC);
    record->type = htonl(type);
    record->phase = htonl(phase);
    record->sequence = htonl(sequence);
}

static int send_all(int socket_fd, const void *data, size_t length)
{
    const unsigned char *bytes = (const unsigned char *)data;
    size_t sent = 0u;

    while (sent < length) {
        int result = send(socket_fd, bytes + sent, length - sent, 0);
        if (result <= 0)
            return 0;
        sent += (size_t)result;
    }
    return 1;
}

static int receive_exact(int socket_fd, void *data, size_t length)
{
    unsigned char *bytes = (unsigned char *)data;
    size_t received = 0u;

    while (received < length) {
        int result = recv(socket_fd, bytes + received, length - received, 0);
        if (result <= 0)
            return 0;
        received += (size_t)result;
    }
    return 1;
}

static int exchange_rounds(int socket_fd, uint32_t phase)
{
    uint32_t sequence;
    struct checkpoint_record sent;
    struct checkpoint_record received;

    for (sequence = 0u; sequence < ROUND_COUNT; sequence++) {
        make_record(&sent, TYPE_PING, phase, sequence);
        if (!send_all(socket_fd, &sent, sizeof(sent)))
            return 0;
        if (!receive_exact(socket_fd, &received, sizeof(received)))
            return 0;

        if (memcmp(&sent, &received, sizeof(sent)) != 0)
            return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    struct checkpoint_record record;
    struct checkpoint_record expected;
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

    if (!exchange_rounds(socket_fd, PHASE_ONE))
        goto done;

    make_record(&record, TYPE_IDLE_READY, 0u, 0u);
    if (!send_all(socket_fd, &record, sizeof(record)))
        goto done;

    make_record(&expected, TYPE_WAKE, 0u, 0u);
    if (!receive_exact(socket_fd, &record, sizeof(record)))
        goto done;
    if (memcmp(&record, &expected, sizeof(record)) != 0)
        goto done;

    make_record(&record, TYPE_WOKE, 0u, 0u);
    if (!send_all(socket_fd, &record, sizeof(record)))
        goto done;

    if (!exchange_rounds(socket_fd, PHASE_TWO))
        goto done;

    make_record(&expected, TYPE_DONE, 0u, 0u);
    if (!receive_exact(socket_fd, &record, sizeof(record)))
        goto done;
    if (memcmp(&record, &expected, sizeof(record)) != 0)
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
