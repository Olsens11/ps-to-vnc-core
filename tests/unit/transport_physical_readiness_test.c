/*
 * Host fixture for the physical-stream readability seam.
 * Uses a real host socketpair so select() behavior is exercised directly while
 * only the PS2 semaphore API is replaced by deterministic local stubs.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "transport_host_stubs/kernel.h"
#include "transport/physical_stream.h"

static int failures;
static int sema_live;

#define CHECK(expr)                                                     \
    do {                                                                \
        if (!(expr)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n",                       \
                    __FILE__, __LINE__, #expr);                         \
            failures++;                                                 \
        }                                                               \
    } while (0)

unsigned char _gp;

int CreateSema(ee_sema_t *semaphore)
{
    if (semaphore == NULL || semaphore->init_count != 1 ||
        semaphore->max_count != 1)
        return -1;
    sema_live = 1;
    return 3;
}

int DeleteSema(int semaphore_id)
{
    if (semaphore_id != 3 || !sema_live)
        return -1;
    sema_live = 0;
    return 0;
}

int WaitSema(int semaphore_id)
{
    return semaphore_id == 3 && sema_live ? 0 : -1;
}

int SignalSema(int semaphore_id)
{
    return semaphore_id == 3 && sema_live ? 0 : -1;
}

static void test_readability_timeout_and_ready(void)
{
    pstvnc_transport_physical_stream_t stream;
    int sockets[2];
    uint8_t byte = 0x5au;

    memset(&stream, 0, sizeof(stream));
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    CHECK(pstvnc_transport_physical_stream_adopt(&stream, sockets[0]) == 1);

    CHECK(pstvnc_transport_physical_stream_wait_readable(&stream, 0u) == 0);
    CHECK(send(sockets[1], &byte, sizeof(byte), 0) == 1);
    CHECK(pstvnc_transport_physical_stream_wait_readable(
        &stream, UINT32_C(100000)) == 1);
    CHECK(recv(sockets[0], &byte, sizeof(byte), 0) == 1);
    CHECK(pstvnc_transport_physical_stream_wait_readable(&stream, 0u) == 0);

    pstvnc_transport_physical_stream_release(&stream);
    close(sockets[1]);
    CHECK(sema_live == 0);
}

static void test_invalid_stream_rejected(void)
{
    pstvnc_transport_physical_stream_t stream;

    memset(&stream, 0, sizeof(stream));
    stream.socket_fd = -1;
    CHECK(pstvnc_transport_physical_stream_wait_readable(&stream, 0u) == -1);
    CHECK(pstvnc_transport_physical_stream_wait_readable(NULL, 0u) == -1);
}

int main(void)
{
    test_readability_timeout_and_ready();
    test_invalid_stream_rejected();

    if (failures != 0) {
        fprintf(stderr,
            "transport_physical_readiness_test: %d failure(s)\n",
            failures);
        return 1;
    }

    puts("transport_physical_readiness_test: PASS");
    return 0;
}
