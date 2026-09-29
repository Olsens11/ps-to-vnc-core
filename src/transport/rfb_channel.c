/*
 * File synopsis:
 * Implements Transport's logical RFB byte-stream storage. Complete PSTV RFB
 * payloads are committed as bytes; parser reads may consume committed bytes
 * incrementally so flow-control credit can return before a large exact read is
 * wholly resident. Terminal residual discard remains distinct from consumption.
 *
 * This file deliberately contains no physical socket, RFB parser, thread,
 * semaphore, credit-policy, or application lifecycle logic.
 *
 * Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md.
 */

#include "rfb_channel.h"

#include <string.h>

int pstvnc_transport_rfb_channel_initialize(
    pstvnc_transport_rfb_channel_t *channel,
    void *storage,
    size_t capacity)
{
    if (channel == NULL || storage == NULL || capacity == 0)
        return -1;

    channel->storage = (uint8_t *)storage;
    channel->capacity = capacity;
    channel->read_offset = 0;
    channel->byte_count = 0;
    channel->activity_generation = 0;
    return 0;
}

int pstvnc_transport_rfb_channel_commit(
    pstvnc_transport_rfb_channel_t *channel,
    const void *payload,
    size_t payload_length)
{
    size_t write_offset;
    size_t first_part;

    if (channel == NULL || channel->storage == NULL ||
        (payload_length != 0 && payload == NULL) ||
        payload_length > channel->capacity - channel->byte_count)
        return -1;

    if (payload_length == 0)
        return 0;

    write_offset = (channel->read_offset + channel->byte_count) % channel->capacity;
    first_part = channel->capacity - write_offset;
    if (first_part > payload_length)
        first_part = payload_length;

    memcpy(channel->storage + write_offset, payload, first_part);
    if (first_part < payload_length)
        memcpy(channel->storage,
               (const uint8_t *)payload + first_part,
               payload_length - first_part);

    channel->byte_count += payload_length;
    channel->activity_generation++;
    return 0;
}

size_t pstvnc_transport_rfb_channel_read_available(
    pstvnc_transport_rfb_channel_t *channel,
    void *buffer,
    size_t maximum_count)
{
    size_t count;
    size_t first_part;

    if (channel == NULL || channel->storage == NULL ||
        (maximum_count != 0 && buffer == NULL))
        return 0;

    count = maximum_count;
    if (count > channel->byte_count)
        count = channel->byte_count;
    if (count == 0)
        return 0;

    first_part = channel->capacity - channel->read_offset;
    if (first_part > count)
        first_part = count;

    memcpy(buffer, channel->storage + channel->read_offset, first_part);
    if (first_part < count)
        memcpy((uint8_t *)buffer + first_part,
               channel->storage,
               count - first_part);

    channel->read_offset = (channel->read_offset + count) % channel->capacity;
    channel->byte_count -= count;
    return count;
}

int pstvnc_transport_rfb_channel_read_exact(
    pstvnc_transport_rfb_channel_t *channel,
    void *buffer,
    size_t count)
{
    if (channel == NULL || channel->storage == NULL ||
        (count != 0 && buffer == NULL) || count > channel->byte_count)
        return -1;

    if (count == 0)
        return 0;

    return pstvnc_transport_rfb_channel_read_available(
               channel, buffer, count) == count
        ? 0
        : -1;
}

int pstvnc_transport_rfb_channel_discard_residual(
    pstvnc_transport_rfb_channel_t *channel,
    size_t expected_count,
    size_t *discarded_count)
{
    if (discarded_count != NULL)
        *discarded_count = 0;

    if (channel == NULL || channel->storage == NULL ||
        channel->byte_count != expected_count)
        return -1;

    if (discarded_count != NULL)
        *discarded_count = channel->byte_count;

    channel->read_offset = 0;
    channel->byte_count = 0;
    return 0;
}

size_t pstvnc_transport_rfb_channel_available(
    const pstvnc_transport_rfb_channel_t *channel)
{
    return channel != NULL ? channel->byte_count : 0;
}

uint32_t pstvnc_transport_rfb_channel_activity_generation(
    const pstvnc_transport_rfb_channel_t *channel)
{
    return channel != NULL ? channel->activity_generation : 0;
}
