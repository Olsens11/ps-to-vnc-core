/*
 * File synopsis:
 * Implements synchronized logical RFB byte storage plus one activity waiter.
 * The wake protocol is extracted from ledge Transport runtime.c: activity is
 * published while the queue lock is held, the waiter is disarmed before the
 * wake token is signaled, and the returning waiter verifies that protected
 * state before accepting the wake.
 *
 * Provenance: provenance/RUNG03B_ACTIVITY_EXTRACTION.md.
 */

#include "rfb_flow.h"

#include <kernel.h>

#include <string.h>

static int create_semaphore(int initial_count, int maximum_count)
{
    ee_sema_t semaphore;

    memset(&semaphore, 0, sizeof(semaphore));
    semaphore.init_count = initial_count;
    semaphore.max_count = maximum_count;
    semaphore.option = 0;
    return CreateSema(&semaphore);
}

static int publish_activity_locked(
    pstvnc_transport_rfb_flow_t *flow)
{
    int signal_waiter = 0;

    flow->activity_sequence++;
    if (flow->activity_wait_armed) {
        flow->activity_wait_armed = 0;
        signal_waiter = 1;
    }

    return signal_waiter;
}

static int signal_activity(
    pstvnc_transport_rfb_flow_t *flow,
    int signal_waiter)
{
    if (!signal_waiter)
        return 1;

    if (SignalSema(flow->activity_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    return 1;
}

int pstvnc_transport_rfb_flow_initialize(
    pstvnc_transport_rfb_flow_t *flow,
    void *storage,
    size_t capacity)
{
    if (flow == NULL || storage == NULL || capacity == 0u)
        return 0;

    memset(flow, 0, sizeof(*flow));
    flow->queue_semaphore_id = -1;
    flow->activity_semaphore_id = -1;

    if (pstvnc_transport_rfb_channel_initialize(
            &flow->channel,
            storage,
            capacity) != 0)
        return 0;

    flow->queue_semaphore_id = create_semaphore(1, 1);
    if (flow->queue_semaphore_id < 0)
        return 0;

    flow->activity_semaphore_id = create_semaphore(0, 1);
    if (flow->activity_semaphore_id < 0) {
        (void)DeleteSema(flow->queue_semaphore_id);
        flow->queue_semaphore_id = -1;
        return 0;
    }

    flow->initialized = 1;
    return 1;
}

int pstvnc_transport_rfb_flow_commit_data(
    pstvnc_transport_rfb_flow_t *flow,
    const void *payload,
    size_t payload_length)
{
    int accepted;
    int signal_waiter;

    if (flow == NULL || !flow->initialized || flow->failed ||
        payload == NULL || payload_length == 0u)
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    accepted = pstvnc_transport_rfb_channel_commit(
        &flow->channel,
        payload,
        payload_length) == 0;

    signal_waiter = accepted
        ? publish_activity_locked(flow)
        : 0;

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    if (!accepted)
        return 0;

    return signal_activity(flow, signal_waiter);
}

int pstvnc_transport_rfb_flow_activity_snapshot(
    pstvnc_transport_rfb_flow_t *flow,
    uint32_t *activity_sequence)
{
    if (flow == NULL || activity_sequence == NULL ||
        !flow->initialized || flow->failed)
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    *activity_sequence = flow->activity_sequence;

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    return 1;
}

int pstvnc_transport_rfb_flow_wait_activity(
    pstvnc_transport_rfb_flow_t *flow,
    uint32_t *activity_sequence)
{
    if (flow == NULL || activity_sequence == NULL ||
        !flow->initialized || flow->failed)
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    if (flow->activity_sequence != *activity_sequence) {
        *activity_sequence = flow->activity_sequence;
        if (SignalSema(flow->queue_semaphore_id) < 0) {
            flow->failed = 1;
            return 0;
        }
        return 1;
    }

    if (flow->activity_wait_armed) {
        (void)SignalSema(flow->queue_semaphore_id);
        flow->failed = 1;
        return 0;
    }

    flow->activity_wait_armed = 1;
    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    if (WaitSema(flow->activity_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    if (flow->activity_wait_armed) {
        flow->activity_wait_armed = 0;
        (void)SignalSema(flow->queue_semaphore_id);
        flow->failed = 1;
        return 0;
    }

    *activity_sequence = flow->activity_sequence;

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    return 1;
}

int pstvnc_transport_rfb_flow_read_exact(
    pstvnc_transport_rfb_flow_t *flow,
    void *buffer,
    size_t count)
{
    int result;

    if (flow == NULL || !flow->initialized || flow->failed ||
        (count != 0u && buffer == NULL))
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    result = pstvnc_transport_rfb_channel_read_exact(
        &flow->channel,
        buffer,
        count) == 0;

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    return result;
}

int pstvnc_transport_rfb_flow_read_available(
    pstvnc_transport_rfb_flow_t *flow,
    void *buffer,
    size_t maximum_count,
    size_t *read_count)
{
    if (read_count != NULL)
        *read_count = 0u;

    if (flow == NULL || read_count == NULL ||
        !flow->initialized || flow->failed ||
        (maximum_count != 0u && buffer == NULL))
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    *read_count = pstvnc_transport_rfb_channel_read_available(
        &flow->channel,
        buffer,
        maximum_count);

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        *read_count = 0u;
        return 0;
    }

    return 1;
}

int pstvnc_transport_rfb_flow_available(
    pstvnc_transport_rfb_flow_t *flow,
    size_t *available_count)
{
    if (available_count != NULL)
        *available_count = 0u;

    if (flow == NULL || available_count == NULL ||
        !flow->initialized || flow->failed)
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        return 0;
    }

    *available_count =
        pstvnc_transport_rfb_channel_available(&flow->channel);

    if (SignalSema(flow->queue_semaphore_id) < 0) {
        flow->failed = 1;
        *available_count = 0u;
        return 0;
    }

    return 1;
}

int pstvnc_transport_rfb_flow_release(
    pstvnc_transport_rfb_flow_t *flow)
{
    int ok = 1;

    if (flow == NULL || !flow->initialized)
        return 0;

    if (WaitSema(flow->queue_semaphore_id) < 0)
        return 0;

    if (flow->activity_wait_armed) {
        (void)SignalSema(flow->queue_semaphore_id);
        return 0;
    }

    if (SignalSema(flow->queue_semaphore_id) < 0)
        return 0;

    if (DeleteSema(flow->activity_semaphore_id) < 0)
        ok = 0;
    if (DeleteSema(flow->queue_semaphore_id) < 0)
        ok = 0;

    if (!ok) {
        flow->failed = 1;
        return 0;
    }

    memset(flow, 0, sizeof(*flow));
    flow->queue_semaphore_id = -1;
    flow->activity_semaphore_id = -1;
    return 1;
}
