/*
 * File synopsis:
 * Implements the backend-independent PSTV fixed-header encoder/decoder, exact
 * provisional Wire-establishment payload codecs, typed RFB provider-terminal
 * codec, exact MPEG generation-control codecs, and explicit frame-identity
 * tests. This file deliberately contains no socket, session allocator, queue,
 * scheduler, prepared-generation state, MPEG producer lifecycle, or PS2-
 * specific policy.
 *
 * Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md; docs/ledge/
 * LEDGE_AUDIT_A003_MPEG_GENERATION.md.
 */

#include "protocol.h"

uint32_t pstvnc_transport_read_be32(const uint8_t input[4])
{
    return ((uint32_t)input[0] << 24) |
           ((uint32_t)input[1] << 16) |
           ((uint32_t)input[2] << 8) |
           (uint32_t)input[3];
}

void pstvnc_transport_write_be32(uint8_t output[4], uint32_t value)
{
    output[0] = (uint8_t)(value >> 24);
    output[1] = (uint8_t)(value >> 16);
    output[2] = (uint8_t)(value >> 8);
    output[3] = (uint8_t)value;
}

int pstvnc_transport_header_encode(
    uint8_t output[PSTVNC_TRANSPORT_HEADER_SIZE],
    const pstvnc_transport_header_t *header)
{
    if (output == NULL || header == NULL ||
        header->version != PSTVNC_TRANSPORT_VERSION ||
        header->payload_length > PSTVNC_TRANSPORT_MAX_PAYLOAD)
        return 0;

    pstvnc_transport_write_be32(&output[0], PSTVNC_TRANSPORT_MAGIC);
    output[4] = header->version;
    output[5] = header->kind;
    output[6] = header->channel;
    output[7] = header->flags;
    pstvnc_transport_write_be32(&output[8], header->sequence);
    pstvnc_transport_write_be32(&output[12], header->payload_length);
    return 1;
}

int pstvnc_transport_header_decode(
    pstvnc_transport_header_t *header,
    const uint8_t input[PSTVNC_TRANSPORT_HEADER_SIZE])
{
    if (header == NULL || input == NULL)
        return 0;
    if (pstvnc_transport_read_be32(&input[0]) != PSTVNC_TRANSPORT_MAGIC)
        return 0;
    if (input[4] != PSTVNC_TRANSPORT_VERSION)
        return 0;

    header->version = input[4];
    header->kind = input[5];
    header->channel = input[6];
    header->flags = input[7];
    header->sequence = pstvnc_transport_read_be32(&input[8]);
    header->payload_length = pstvnc_transport_read_be32(&input[12]);

    return header->payload_length <= PSTVNC_TRANSPORT_MAX_PAYLOAD;
}


static int pstvnc_wire_not_accepted_reason_valid(uint32_t reason)
{
    return reason == (uint32_t)PSTVNC_WIRE_NOT_ACCEPTED_WIRE_VERSION ||
        reason == (uint32_t)PSTVNC_WIRE_NOT_ACCEPTED_PRODUCT_VERSION ||
        reason == (uint32_t)PSTVNC_WIRE_NOT_ACCEPTED_MALFORMED;
}

static int pstvnc_rfb_provider_failure_reason_valid(uint32_t reason)
{
    return reason == (uint32_t)PSTVNC_RFB_PROVIDER_FAILURE_CONNECT ||
        reason == (uint32_t)PSTVNC_RFB_PROVIDER_FAILURE_READ ||
        reason == (uint32_t)PSTVNC_RFB_PROVIDER_FAILURE_WRITE;
}

int pstvnc_wire_hello_payload_encode(
    uint8_t output[PSTVNC_WIRE_HELLO_PAYLOAD_SIZE],
    const pstvnc_wire_hello_payload_t *payload)
{
    if (output == NULL || payload == NULL)
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->wire_version);
    pstvnc_transport_write_be32(
        &output[4],
        payload->product_establishment_version);
    return 1;
}

int pstvnc_wire_hello_payload_decode(
    pstvnc_wire_hello_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_WIRE_HELLO_PAYLOAD_SIZE)
        return 0;

    payload->wire_version = pstvnc_transport_read_be32(&input[0]);
    payload->product_establishment_version =
        pstvnc_transport_read_be32(&input[4]);
    return 1;
}

int pstvnc_wire_accept_payload_encode(
    uint8_t output[PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE],
    const pstvnc_wire_accept_payload_t *payload)
{
    if (output == NULL || payload == NULL || payload->session_id == 0u)
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->session_id);
    return 1;
}

int pstvnc_wire_accept_payload_decode(
    pstvnc_wire_accept_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE)
        return 0;

    payload->session_id = pstvnc_transport_read_be32(&input[0]);
    return payload->session_id != 0u;
}

int pstvnc_wire_not_accepted_payload_encode(
    uint8_t output[PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE],
    const pstvnc_wire_not_accepted_payload_t *payload)
{
    if (output == NULL || payload == NULL ||
        !pstvnc_wire_not_accepted_reason_valid(payload->reason))
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->reason);
    return 1;
}

int pstvnc_wire_not_accepted_payload_decode(
    pstvnc_wire_not_accepted_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE)
        return 0;

    payload->reason = pstvnc_transport_read_be32(&input[0]);
    return pstvnc_wire_not_accepted_reason_valid(payload->reason);
}

int pstvnc_rfb_provider_failure_payload_encode(
    uint8_t output[PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE],
    const pstvnc_rfb_provider_failure_payload_t *payload)
{
    if (output == NULL || payload == NULL ||
        !pstvnc_rfb_provider_failure_reason_valid(payload->reason))
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->reason);
    return 1;
}

int pstvnc_rfb_provider_failure_payload_decode(
    pstvnc_rfb_provider_failure_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE)
        return 0;

    payload->reason = pstvnc_transport_read_be32(&input[0]);
    return pstvnc_rfb_provider_failure_reason_valid(payload->reason);
}

int pstvnc_mpeg_retire_payload_encode(
    uint8_t output[PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE],
    const pstvnc_mpeg_retire_payload_t *payload)
{
    if (output == NULL || payload == NULL ||
        payload->version != PSTVNC_MPEG_GENERATION_CONTROL_VERSION)
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->version);
    pstvnc_transport_write_be32(&output[4], payload->session_id);
    pstvnc_transport_write_be32(&output[8], payload->generation);
    return 1;
}

int pstvnc_mpeg_retire_payload_decode(
    pstvnc_mpeg_retire_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE)
        return 0;

    payload->version = pstvnc_transport_read_be32(&input[0]);
    payload->session_id = pstvnc_transport_read_be32(&input[4]);
    payload->generation = pstvnc_transport_read_be32(&input[8]);
    return payload->version == PSTVNC_MPEG_GENERATION_CONTROL_VERSION;
}

int pstvnc_mpeg_start_payload_encode(
    uint8_t output[PSTVNC_MPEG_START_PAYLOAD_SIZE],
    const pstvnc_mpeg_start_payload_t *payload)
{
    if (output == NULL || payload == NULL ||
        payload->version != PSTVNC_MPEG_GENERATION_CONTROL_VERSION)
        return 0;

    pstvnc_transport_write_be32(&output[0], payload->version);
    pstvnc_transport_write_be32(&output[4], payload->session_id);
    pstvnc_transport_write_be32(&output[8], payload->generation);
    pstvnc_transport_write_be32(&output[12], payload->base_x);
    pstvnc_transport_write_be32(&output[16], payload->base_y);
    pstvnc_transport_write_be32(&output[20], payload->base_width);
    pstvnc_transport_write_be32(&output[24], payload->base_height);
    pstvnc_transport_write_be32(&output[28], payload->suppression_x);
    pstvnc_transport_write_be32(&output[32], payload->suppression_y);
    pstvnc_transport_write_be32(&output[36], payload->suppression_width);
    pstvnc_transport_write_be32(&output[40], payload->suppression_height);
    return 1;
}

int pstvnc_mpeg_start_payload_decode(
    pstvnc_mpeg_start_payload_t *payload,
    const uint8_t *input,
    size_t input_size)
{
    if (payload == NULL || input == NULL ||
        input_size != PSTVNC_MPEG_START_PAYLOAD_SIZE)
        return 0;

    payload->version = pstvnc_transport_read_be32(&input[0]);
    payload->session_id = pstvnc_transport_read_be32(&input[4]);
    payload->generation = pstvnc_transport_read_be32(&input[8]);
    payload->base_x = pstvnc_transport_read_be32(&input[12]);
    payload->base_y = pstvnc_transport_read_be32(&input[16]);
    payload->base_width = pstvnc_transport_read_be32(&input[20]);
    payload->base_height = pstvnc_transport_read_be32(&input[24]);
    payload->suppression_x = pstvnc_transport_read_be32(&input[28]);
    payload->suppression_y = pstvnc_transport_read_be32(&input[32]);
    payload->suppression_width = pstvnc_transport_read_be32(&input[36]);
    payload->suppression_height = pstvnc_transport_read_be32(&input[40]);
    return payload->version == PSTVNC_MPEG_GENERATION_CONTROL_VERSION;
}


int pstvnc_transport_header_is_wire_hello(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_HELLO &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_CONTROL &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_WIRE_HELLO_PAYLOAD_SIZE;
}

int pstvnc_transport_header_is_wire_accept(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_ACCEPT &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_CONTROL &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE;
}

int pstvnc_transport_header_is_wire_not_accepted(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_NOT_ACCEPTED &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_CONTROL &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE;
}

int pstvnc_transport_header_is_rfb_provider_failure(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_ERROR &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_RFB &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE;
}

int pstvnc_transport_header_is_mpeg_data(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_DATA &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_MPEG2 &&
        header->flags == 0u &&
        header->payload_length != 0u &&
        header->payload_length <= PSTVNC_TRANSPORT_MAX_PAYLOAD;
}

int pstvnc_transport_header_is_mpeg_retire(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_MPEG_RETIRE &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_CONTROL &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE;
}

int pstvnc_transport_header_is_mpeg_start(
    const pstvnc_transport_header_t *header)
{
    return header != NULL &&
        header->kind == PSTVNC_TRANSPORT_FRAME_MPEG_START &&
        header->channel == PSTVNC_TRANSPORT_CHANNEL_CONTROL &&
        header->flags == 0u &&
        header->payload_length == PSTVNC_MPEG_START_PAYLOAD_SIZE;
}
