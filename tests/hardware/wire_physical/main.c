/*
 * Hardware checkpoint 02: physical PSTV Wire framing and receive readiness.
 *
 * Exercises only the qualified platform/network foundation plus imported
 * transport/protocol and transport/physical_stream. Logical rider semantics,
 * queues, dispatch, RFB parsing, audio, MPEG, and application runtime are absent.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "platform/ps2_network.h"
#include "platform/ps2_system.h"
#include "transport/physical_stream.h"
#include "transport/protocol.h"

#define ACTIVE_ROUNDS 128u
#define ACTIVE_POLL_LIMIT 5000u
#define IDLE_POLL_LIMIT 70000u
#define READINESS_TIMEOUT_US 1000u
#define EMPTY_POLL_YIELD_US 1000u

static uint8_t send_payload[PSTVNC_TRANSPORT_MAX_PAYLOAD];
static uint8_t receive_payload[PSTVNC_TRANSPORT_MAX_PAYLOAD];

static const size_t payload_sizes[] = {
    0u, 1u, 2u, 3u, 7u, 15u, 16u, 17u,
    31u, 63u, 64u, 65u, 127u, 255u, 256u, 257u,
    511u, 1024u, 2048u, 4096u, PSTVNC_TRANSPORT_MAX_PAYLOAD
};

static void fill_payload(uint32_t phase, uint32_t round, size_t length)
{
    size_t index;

    for (index = 0u; index < length; index++) {
        send_payload[index] = (uint8_t)(
            (phase * 0x31u) ^
            (round * 0x17u) ^
            ((uint32_t)index * 0x07u));
    }
}

static int wait_until_readable(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    uint32_t polls = 0u;

    while (polls < poll_limit) {
        int result = pstvnc_transport_physical_stream_wait_readable(
            stream,
            READINESS_TIMEOUT_US);

        if (result > 0) {
            if (empty_polls != NULL)
                *empty_polls = polls;
            return 1;
        }
        if (result < 0)
            return 0;

        polls++;
        if (pstvnc_ps2_system_delay_us(EMPTY_POLL_YIELD_US) < 0)
            return 0;
    }

    return 0;
}

static int receive_data_frame(
    pstvnc_transport_physical_stream_t *stream,
    const uint8_t *expected_payload,
    size_t expected_length,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    pstvnc_transport_header_t header;

    if (!wait_until_readable(stream, poll_limit, empty_polls))
        return 0;

    if (!pstvnc_transport_physical_stream_receive_frame(
            stream,
            &header,
            receive_payload,
            sizeof(receive_payload)))
        return 0;

    if (header.kind != PSTVNC_TRANSPORT_FRAME_DATA ||
        header.channel != PSTVNC_TRANSPORT_CHANNEL_CONTROL ||
        header.flags != 0u ||
        header.payload_length != expected_length)
        return 0;

    if (expected_length > 0u &&
        memcmp(receive_payload, expected_payload, expected_length) != 0)
        return 0;

    return 1;
}

static int run_active_phase(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t phase)
{
    uint32_t round;

    for (round = 0u; round < ACTIVE_ROUNDS; round++) {
        size_t length =
            payload_sizes[round % (sizeof(payload_sizes) / sizeof(payload_sizes[0]))];

        fill_payload(phase, round, length);

        if (!pstvnc_transport_physical_stream_send_frame(
                stream,
                PSTVNC_TRANSPORT_FRAME_DATA,
                PSTVNC_TRANSPORT_CHANNEL_CONTROL,
                0u,
                send_payload,
                length))
            return 0;

        if (!receive_data_frame(
                stream,
                send_payload,
                length,
                ACTIVE_POLL_LIMIT,
                NULL))
            return 0;
    }

    return 1;
}

static int send_marker(
    pstvnc_transport_physical_stream_t *stream,
    const char *marker,
    size_t marker_length)
{
    return pstvnc_transport_physical_stream_send_frame(
        stream,
        PSTVNC_TRANSPORT_FRAME_DATA,
        PSTVNC_TRANSPORT_CHANNEL_CONTROL,
        0u,
        marker,
        marker_length);
}

static int receive_marker(
    pstvnc_transport_physical_stream_t *stream,
    const char *marker,
    size_t marker_length,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    return receive_data_frame(
        stream,
        (const uint8_t *)marker,
        marker_length,
        poll_limit,
        empty_polls);
}

static int run_idle_wake(
    pstvnc_transport_physical_stream_t *stream)
{
    static const char idle_ready[] = "WIRE_IDLE_READY";
    static const char wake[] = "WIRE_WAKE";
    uint8_t woke_payload[8];
    uint32_t empty_polls = 0u;

    if (!send_marker(
            stream,
            idle_ready,
            sizeof(idle_ready) - 1u))
        return 0;

    if (!receive_marker(
            stream,
            wake,
            sizeof(wake) - 1u,
            IDLE_POLL_LIMIT,
            &empty_polls))
        return 0;

    memcpy(woke_payload, "WOKE", 4u);
    pstvnc_transport_write_be32(&woke_payload[4], empty_polls);

    return pstvnc_transport_physical_stream_send_frame(
        stream,
        PSTVNC_TRANSPORT_FRAME_DATA,
        PSTVNC_TRANSPORT_CHANNEL_CONTROL,
        0u,
        woke_payload,
        sizeof(woke_payload));
}

int main(int argc, char **argv)
{
    static const char pass_marker[] = "WIRE_PASS";
    static const char done_marker[] = "WIRE_DONE";
    pstvnc_transport_physical_stream_t stream;
    pstvnc_wire_not_accepted_reason_t rejection_reason;
    uint32_t session_id = 0u;
    int socket_fd = -1;
    int stream_live = 0;
    int success = 0;

    (void)argc;
    (void)argv;
    memset(&stream, 0, sizeof(stream));

    if (pstvnc_ps2_system_prepare_iop() < 0)
        goto done;
    if (pstvnc_ps2_network_init() < 0)
        goto done;
    if (pstvnc_ps2_network_wait_link() < 0)
        goto done;

    socket_fd = pstvnc_ps2_network_connect_management();
    if (socket_fd < 0)
        goto done;

    rejection_reason = (pstvnc_wire_not_accepted_reason_t)0;
    if (pstvnc_transport_physical_stream_establish_client(
            &stream,
            &socket_fd,
            &session_id,
            &rejection_reason) != 1)
        goto done;

    stream_live = 1;
    if (session_id == 0u)
        goto done;

    if (!run_active_phase(&stream, 1u))
        goto done;

    if (!run_idle_wake(&stream))
        goto done;

    if (!run_active_phase(&stream, 2u))
        goto done;

    if (!send_marker(
            &stream,
            pass_marker,
            sizeof(pass_marker) - 1u))
        goto done;

    if (!receive_marker(
            &stream,
            done_marker,
            sizeof(done_marker) - 1u,
            ACTIVE_POLL_LIMIT,
            NULL))
        goto done;

    success = 1;

done:
    if (stream_live)
        pstvnc_transport_physical_stream_release(&stream);
    if (socket_fd >= 0)
        pstvnc_ps2_network_close(socket_fd);

    if (!success)
        return 1;

    pstvnc_ps2_system_exit_to_menu();
    return 0;
}
