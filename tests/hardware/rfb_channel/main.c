/*
 * Hardware checkpoint 03A: logical RFB byte-channel storage.
 *
 * Exercises only the qualified platform/network + physical Wire foundation and
 * the imported transport/rfb_channel module. Credit policy, activity semaphore
 * rendezvous, runtime dispatch, and the RFB parser are deliberately absent.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "platform/ps2_network.h"
#include "platform/ps2_system.h"
#include "transport/physical_stream.h"
#include "transport/protocol.h"
#include "transport/rfb_channel.h"

#define QUEUE_CAPACITY 1024u
#define FRAME_CAPACITY PSTVNC_TRANSPORT_MAX_PAYLOAD
#define ACTIVE_POLL_LIMIT 5000u
#define IDLE_POLL_LIMIT 70000u
#define READINESS_TIMEOUT_US 1000u
#define EMPTY_POLL_YIELD_US 1000u

static uint8_t queue_storage[QUEUE_CAPACITY];
static uint8_t frame_payload[FRAME_CAPACITY];
static uint8_t read_buffer[QUEUE_CAPACITY];

static uint8_t expected_byte(uint32_t stream_offset)
{
    return (uint8_t)(((stream_offset * 13u) + 0x5au) & 0xffu);
}

static int validate_bytes(
    const uint8_t *bytes,
    size_t length,
    uint32_t stream_offset)
{
    size_t index;

    for (index = 0u; index < length; index++) {
        if (bytes[index] != expected_byte(stream_offset + (uint32_t)index))
            return 0;
    }

    return 1;
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

static int receive_frame(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_header_t *header,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    if (!wait_until_readable(stream, poll_limit, empty_polls))
        return 0;

    return pstvnc_transport_physical_stream_receive_frame(
        stream,
        header,
        frame_payload,
        sizeof(frame_payload));
}

static int receive_rfb_data(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_rfb_channel_t *channel,
    size_t expected_length,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    pstvnc_transport_header_t header;

    if (!receive_frame(stream, &header, poll_limit, empty_polls))
        return 0;

    if (header.kind != PSTVNC_TRANSPORT_FRAME_DATA ||
        header.channel != PSTVNC_TRANSPORT_CHANNEL_RFB ||
        header.flags != 0u ||
        header.payload_length != expected_length)
        return 0;

    return pstvnc_transport_rfb_channel_commit(
               channel,
               frame_payload,
               header.payload_length) == 0;
}

static int send_control(
    pstvnc_transport_physical_stream_t *stream,
    const void *payload,
    size_t payload_length)
{
    return pstvnc_transport_physical_stream_send_frame(
        stream,
        PSTVNC_TRANSPORT_FRAME_DATA,
        PSTVNC_TRANSPORT_CHANNEL_CONTROL,
        0u,
        payload,
        payload_length);
}

static int receive_control(
    pstvnc_transport_physical_stream_t *stream,
    const void *expected,
    size_t expected_length)
{
    pstvnc_transport_header_t header;

    if (!receive_frame(stream, &header, ACTIVE_POLL_LIMIT, NULL))
        return 0;

    if (header.kind != PSTVNC_TRANSPORT_FRAME_DATA ||
        header.channel != PSTVNC_TRANSPORT_CHANNEL_CONTROL ||
        header.flags != 0u ||
        header.payload_length != expected_length)
        return 0;

    return expected_length == 0u ||
        memcmp(frame_payload, expected, expected_length) == 0;
}

static int read_available_and_validate(
    pstvnc_transport_rfb_channel_t *channel,
    size_t maximum_count,
    size_t expected_count,
    uint32_t stream_offset)
{
    size_t taken;

    memset(read_buffer, 0, sizeof(read_buffer));
    taken = pstvnc_transport_rfb_channel_read_available(
        channel,
        read_buffer,
        maximum_count);

    return taken == expected_count &&
        validate_bytes(read_buffer, taken, stream_offset);
}

static int read_exact_and_validate(
    pstvnc_transport_rfb_channel_t *channel,
    size_t count,
    uint32_t stream_offset)
{
    memset(read_buffer, 0, sizeof(read_buffer));

    return pstvnc_transport_rfb_channel_read_exact(
               channel,
               read_buffer,
               count) == 0 &&
        validate_bytes(read_buffer, count, stream_offset);
}

static int run_queue_sequence(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_rfb_channel_t *channel)
{
    size_t discarded = 99u;

    if (pstvnc_transport_rfb_channel_available(channel) != 0u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 0u)
        return 0;

    if (!receive_rfb_data(stream, channel, 700u, ACTIVE_POLL_LIMIT, NULL) ||
        !validate_bytes(frame_payload, 700u, 0u) ||
        pstvnc_transport_rfb_channel_available(channel) != 700u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 1u)
        return 0;

    if (!read_available_and_validate(channel, 600u, 600u, 0u) ||
        pstvnc_transport_rfb_channel_available(channel) != 100u)
        return 0;

    if (!receive_rfb_data(stream, channel, 800u, ACTIVE_POLL_LIMIT, NULL) ||
        !validate_bytes(frame_payload, 800u, 700u) ||
        pstvnc_transport_rfb_channel_available(channel) != 900u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 2u)
        return 0;

    if (!read_exact_and_validate(channel, 850u, 600u) ||
        pstvnc_transport_rfb_channel_available(channel) != 50u)
        return 0;

    if (!receive_rfb_data(stream, channel, 900u, ACTIVE_POLL_LIMIT, NULL) ||
        !validate_bytes(frame_payload, 900u, 1500u) ||
        pstvnc_transport_rfb_channel_available(channel) != 950u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 3u)
        return 0;

    if (!read_available_and_validate(channel, QUEUE_CAPACITY, 950u, 1450u) ||
        pstvnc_transport_rfb_channel_available(channel) != 0u)
        return 0;

    if (!receive_rfb_data(stream, channel, 0u, ACTIVE_POLL_LIMIT, NULL) ||
        pstvnc_transport_rfb_channel_available(channel) != 0u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 3u)
        return 0;

    if (!receive_rfb_data(stream, channel, 3u, ACTIVE_POLL_LIMIT, NULL) ||
        !validate_bytes(frame_payload, 3u, 2400u) ||
        pstvnc_transport_rfb_channel_available(channel) != 3u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 4u)
        return 0;

    memset(read_buffer, 0xaau, 4u);
    if (pstvnc_transport_rfb_channel_read_exact(
            channel,
            read_buffer,
            4u) == 0 ||
        pstvnc_transport_rfb_channel_available(channel) != 3u ||
        read_buffer[0] != 0xaau ||
        read_buffer[1] != 0xaau ||
        read_buffer[2] != 0xaau ||
        read_buffer[3] != 0xaau)
        return 0;

    if (!read_exact_and_validate(channel, 3u, 2400u) ||
        pstvnc_transport_rfb_channel_available(channel) != 0u)
        return 0;

    if (!receive_rfb_data(stream, channel, 5u, ACTIVE_POLL_LIMIT, NULL) ||
        !validate_bytes(frame_payload, 5u, 2403u) ||
        pstvnc_transport_rfb_channel_available(channel) != 5u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 5u)
        return 0;

    if (pstvnc_transport_rfb_channel_discard_residual(
            channel,
            4u,
            &discarded) == 0 ||
        discarded != 0u ||
        pstvnc_transport_rfb_channel_available(channel) != 5u)
        return 0;

    if (pstvnc_transport_rfb_channel_discard_residual(
            channel,
            5u,
            &discarded) != 0 ||
        discarded != 5u ||
        pstvnc_transport_rfb_channel_available(channel) != 0u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 5u)
        return 0;

    return 1;
}

static int run_idle_wake(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_rfb_channel_t *channel)
{
    static const char idle_ready[] = "RFB03A_IDLE_READY";
    static const char done[] = "RFB03A_DONE";
    uint8_t result_payload[16];
    uint32_t empty_polls = 0u;

    if (!send_control(
            stream,
            idle_ready,
            sizeof(idle_ready) - 1u))
        return 0;

    if (!receive_rfb_data(
            stream,
            channel,
            257u,
            IDLE_POLL_LIMIT,
            &empty_polls) ||
        !validate_bytes(frame_payload, 257u, 2408u) ||
        pstvnc_transport_rfb_channel_available(channel) != 257u ||
        pstvnc_transport_rfb_channel_activity_generation(channel) != 6u)
        return 0;

    if (!read_exact_and_validate(channel, 257u, 2408u) ||
        pstvnc_transport_rfb_channel_available(channel) != 0u)
        return 0;

    memcpy(result_payload, "R03A", 4u);
    pstvnc_transport_write_be32(&result_payload[4], empty_polls);
    pstvnc_transport_write_be32(
        &result_payload[8],
        pstvnc_transport_rfb_channel_activity_generation(channel));
    pstvnc_transport_write_be32(
        &result_payload[12],
        (uint32_t)pstvnc_transport_rfb_channel_available(channel));

    if (!send_control(stream, result_payload, sizeof(result_payload)))
        return 0;

    return receive_control(stream, done, sizeof(done) - 1u);
}

int main(int argc, char **argv)
{
    pstvnc_transport_physical_stream_t stream;
    pstvnc_transport_rfb_channel_t channel;
    pstvnc_wire_not_accepted_reason_t rejection_reason;
    uint32_t session_id = 0u;
    int socket_fd = -1;
    int stream_live = 0;
    int success = 0;

    (void)argc;
    (void)argv;
    memset(&stream, 0, sizeof(stream));
    memset(&channel, 0, sizeof(channel));

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

    if (session_id == 0u ||
        pstvnc_transport_rfb_channel_initialize(
            &channel,
            queue_storage,
            sizeof(queue_storage)) != 0)
        goto done;

    if (!run_queue_sequence(&stream, &channel))
        goto done;

    if (!run_idle_wake(&stream, &channel))
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
