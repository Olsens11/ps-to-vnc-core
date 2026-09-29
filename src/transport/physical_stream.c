/*
 * File synopsis:
 * Implements ownership of the one adopted physical PSTV socket, the single
 * serialized framed-send path, the sole framed-receive sequencing path, and
 * the exact Transport-internal Q4 client establishment transaction. This file
 * deliberately does not dispatch logical channels, parse RFB, manage media
 * policy, or decide product lifecycle; those remain documented owners.
 *
 * Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md.
 */

#include "physical_stream.h"

#include <kernel.h>
#include <ps2ip.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int pstvnc_transport_physical_stream_send_exact(
    int socket_fd,
    const uint8_t *bytes,
    size_t byte_count)
{
    size_t sent_total = 0;

    while (sent_total < byte_count) {
        int sent = send(socket_fd, bytes + sent_total, byte_count - sent_total, 0);

        if (sent <= 0)
            return 0;

        sent_total += (size_t)sent;
    }

    return 1;
}

static int pstvnc_transport_physical_stream_receive_exact(
    int socket_fd,
    uint8_t *bytes,
    size_t byte_count)
{
    size_t received_total = 0;

    while (received_total < byte_count) {
        int received = recv(
            socket_fd,
            bytes + received_total,
            byte_count - received_total,
            0);

        if (received <= 0)
            return 0;

        received_total += (size_t)received;
    }

    return 1;
}

int pstvnc_transport_physical_stream_adopt(
    pstvnc_transport_physical_stream_t *stream,
    int socket_fd)
{
    ee_sema_t send_mutex;
    int send_semaphore_id;

    if (stream == NULL || socket_fd < 0)
        return 0;

    stream->socket_fd = -1;
    stream->send_semaphore_id = -1;
    stream->next_send_sequence = 1u;
    stream->expected_receive_sequence = 1u;

    memset(&send_mutex, 0, sizeof(send_mutex));
    send_mutex.init_count = 1;
    send_mutex.max_count = 1;
    send_mutex.option = 0;

    send_semaphore_id = CreateSema(&send_mutex);
    if (send_semaphore_id < 0)
        return 0;

    stream->socket_fd = socket_fd;
    stream->send_semaphore_id = send_semaphore_id;
    return 1;
}

int pstvnc_transport_physical_stream_establish_client(
    pstvnc_transport_physical_stream_t *stream,
    int *socket_fd,
    uint32_t *session_id,
    pstvnc_wire_not_accepted_reason_t *rejection_reason)
{
    uint8_t payload[PSTVNC_WIRE_HELLO_PAYLOAD_SIZE];
    pstvnc_transport_header_t header;
    pstvnc_wire_hello_payload_t hello;
    int result = 0;

    if (stream == NULL || socket_fd == NULL || *socket_fd < 0 ||
        session_id == NULL || rejection_reason == NULL)
        return 0;

    *session_id = 0u;
    *rejection_reason = (pstvnc_wire_not_accepted_reason_t)0;

    if (!pstvnc_transport_physical_stream_adopt(stream, *socket_fd))
        return 0;

    /*
     * Descriptor ownership transfers at successful adoption, before any Wire
     * byte is sent. From here on, every failure path closes through Transport,
     * so Application/Platform can never race a second close.
     */
    *socket_fd = -1;

    hello.wire_version = PSTVNC_TRANSPORT_VERSION;
    hello.product_establishment_version =
        PSTVNC_WIRE_PRODUCT_ESTABLISHMENT_VERSION;

    if (!pstvnc_wire_hello_payload_encode(payload, &hello) ||
        !pstvnc_transport_physical_stream_send_frame(
            stream,
            PSTVNC_TRANSPORT_FRAME_HELLO,
            PSTVNC_TRANSPORT_CHANNEL_CONTROL,
            0u,
            payload,
            PSTVNC_WIRE_HELLO_PAYLOAD_SIZE))
        goto fail;

    /*
     * send_frame() consumed outbound sequence 1. The matching receive path must
     * now consume exactly inbound sequence 1; only a complete result advances
     * expected_receive_sequence to 2.
     */
    if (!pstvnc_transport_physical_stream_receive_frame(
            stream,
            &header,
            payload,
            sizeof(payload)))
        goto fail;

    if (pstvnc_transport_header_is_wire_accept(&header)) {
        pstvnc_wire_accept_payload_t acceptance;

        if (!pstvnc_wire_accept_payload_decode(
                &acceptance,
                payload,
                header.payload_length))
            goto fail;

        *session_id = acceptance.session_id;
        result = 1;
    } else if (pstvnc_transport_header_is_wire_not_accepted(&header)) {
        pstvnc_wire_not_accepted_payload_t rejection;

        if (!pstvnc_wire_not_accepted_payload_decode(
                &rejection,
                payload,
                header.payload_length))
            goto fail;

        *rejection_reason =
            (pstvnc_wire_not_accepted_reason_t)rejection.reason;
        result = -1;
    } else {
        goto fail;
    }

    if (result == 1) {
        /*
         * Do not permit a hidden sequence reset between Q4 and later riders.
         * These are the exact continuation values the runtime must inherit.
         */
        if (stream->next_send_sequence != 2u ||
            stream->expected_receive_sequence != 2u)
            goto fail;
        return 1;
    }

    /* NOT_ACCEPTED is a typed protocol outcome but owns no live session. */
    pstvnc_transport_physical_stream_release(stream);
    return -1;

fail:
    pstvnc_transport_physical_stream_release(stream);
    *session_id = 0u;
    *rejection_reason = (pstvnc_wire_not_accepted_reason_t)0;
    return 0;
}

int pstvnc_transport_physical_stream_transfer_established(
    pstvnc_transport_physical_stream_t *destination,
    pstvnc_transport_physical_stream_t *source)
{
    if (destination == NULL || source == NULL || destination == source ||
        source->socket_fd < 0 || source->send_semaphore_id < 0 ||
        source->next_send_sequence != 2u ||
        source->expected_receive_sequence != 2u)
        return 0;

    /*
     * This is an ownership move, not sequence seeding. The exact physical
     * lineage and its send lock move together; callers cannot provide arbitrary
     * next-sequence values.
     */
    *destination = *source;

    source->socket_fd = -1;
    source->send_semaphore_id = -1;
    source->next_send_sequence = 1u;
    source->expected_receive_sequence = 1u;
    return 1;
}

int pstvnc_transport_physical_stream_send_frame(
    pstvnc_transport_physical_stream_t *stream,
    uint8_t kind,
    uint8_t channel,
    uint8_t flags,
    const void *payload,
    size_t payload_length)
{
    uint8_t wire_header[PSTVNC_TRANSPORT_HEADER_SIZE];
    pstvnc_transport_header_t header;
    int send_succeeded = 0;

    if (stream == NULL || stream->socket_fd < 0 ||
        stream->send_semaphore_id < 0 ||
        payload_length > PSTVNC_TRANSPORT_MAX_PAYLOAD ||
        (payload_length > 0 && payload == NULL))
        return 0;

    if (WaitSema(stream->send_semaphore_id) < 0)
        return 0;

    memset(&header, 0, sizeof(header));
    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = kind;
    header.channel = channel;
    header.flags = flags;
    header.sequence = stream->next_send_sequence;
    header.payload_length = (uint32_t)payload_length;

    if (pstvnc_transport_header_encode(wire_header, &header) &&
        pstvnc_transport_physical_stream_send_exact(
            stream->socket_fd, wire_header, sizeof(wire_header)) &&
        (payload_length == 0 ||
         pstvnc_transport_physical_stream_send_exact(
             stream->socket_fd, (const uint8_t *)payload, payload_length))) {
        /* Advance only after the complete frame has reached the socket API. */
        stream->next_send_sequence++;
        send_succeeded = 1;
    }

    if (SignalSema(stream->send_semaphore_id) < 0)
        return 0;

    return send_succeeded;
}

int pstvnc_transport_physical_stream_wait_readable(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t timeout_us)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    if (stream == NULL || stream->socket_fd < 0)
        return -1;

    FD_ZERO(&read_set);
    FD_SET(stream->socket_fd, &read_set);

    timeout.tv_sec = (long)(timeout_us / UINT32_C(1000000));
    timeout.tv_usec = (long)(timeout_us % UINT32_C(1000000));

    result = select(
        stream->socket_fd + 1,
        &read_set,
        NULL,
        NULL,
        &timeout);

    if (result < 0)
        return -1;
    if (result == 0)
        return 0;

    return FD_ISSET(stream->socket_fd, &read_set) ? 1 : -1;
}

int pstvnc_transport_physical_stream_receive_frame(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_header_t *header,
    void *payload,
    size_t payload_capacity)
{
    uint8_t wire_header[PSTVNC_TRANSPORT_HEADER_SIZE];

    if (stream == NULL || stream->socket_fd < 0 || header == NULL)
        return 0;

    if (!pstvnc_transport_physical_stream_receive_exact(
            stream->socket_fd, wire_header, sizeof(wire_header)) ||
        !pstvnc_transport_header_decode(header, wire_header) ||
        header->sequence != stream->expected_receive_sequence ||
        header->payload_length > payload_capacity ||
        (header->payload_length > 0u && payload == NULL))
        return 0;

    if (header->payload_length > 0u &&
        !pstvnc_transport_physical_stream_receive_exact(
            stream->socket_fd,
            (uint8_t *)payload,
            header->payload_length))
        return 0;

    /* A frame becomes ordered receive authority only when it is complete. */
    stream->expected_receive_sequence++;
    return 1;
}

int pstvnc_transport_physical_stream_shutdown_io(
    pstvnc_transport_physical_stream_t *stream)
{
    if (stream == NULL || stream->socket_fd < 0)
        return 0;

    /*
     * This is intentionally not close(). Transport retains the descriptor and
     * releases it only after the sole receiver has published completion.
     */
    return shutdown(stream->socket_fd, SHUT_RDWR) == 0;
}

void pstvnc_transport_physical_stream_release(
    pstvnc_transport_physical_stream_t *stream)
{
    if (stream == NULL)
        return;

    if (stream->socket_fd >= 0)
        close(stream->socket_fd);
    if (stream->send_semaphore_id >= 0)
        DeleteSema(stream->send_semaphore_id);

    stream->socket_fd = -1;
    stream->send_semaphore_id = -1;
    stream->next_send_sequence = 1u;
    stream->expected_receive_sequence = 1u;
}