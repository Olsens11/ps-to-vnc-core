/*
 * File synopsis:
 * Defines the clean reconstruction's synchronized logical RFB byte channel.
 * It combines the admitted rfb_channel storage primitive with the exact
 * single-waiter activity sequence/rendezvous discipline extracted from ledge
 * Transport runtime.c.
 *
 * This module owns no socket, physical Wire I/O, credit policy, provider
 * terminal state, parser behavior, or application lifecycle.
 *
 * Provenance: provenance/RUNG03B_ACTIVITY_EXTRACTION.md.
 */

#ifndef PSTVNC_TRANSPORT_RFB_FLOW_H
#define PSTVNC_TRANSPORT_RFB_FLOW_H

#include "rfb_channel.h"

#include <stddef.h>
#include <stdint.h>

typedef struct pstvnc_transport_rfb_flow {
    pstvnc_transport_rfb_channel_t channel;
    int queue_semaphore_id;
    int activity_semaphore_id;
    uint32_t activity_sequence;
    int activity_wait_armed;
    int failed;
    int initialized;
} pstvnc_transport_rfb_flow_t;

int pstvnc_transport_rfb_flow_initialize(
    pstvnc_transport_rfb_flow_t *flow,
    void *storage,
    size_t capacity);

int pstvnc_transport_rfb_flow_commit_data(
    pstvnc_transport_rfb_flow_t *flow,
    const void *payload,
    size_t payload_length);

int pstvnc_transport_rfb_flow_activity_snapshot(
    pstvnc_transport_rfb_flow_t *flow,
    uint32_t *activity_sequence);

int pstvnc_transport_rfb_flow_wait_activity(
    pstvnc_transport_rfb_flow_t *flow,
    uint32_t *activity_sequence);

int pstvnc_transport_rfb_flow_read_exact(
    pstvnc_transport_rfb_flow_t *flow,
    void *buffer,
    size_t count);

int pstvnc_transport_rfb_flow_read_available(
    pstvnc_transport_rfb_flow_t *flow,
    void *buffer,
    size_t maximum_count,
    size_t *read_count);

int pstvnc_transport_rfb_flow_available(
    pstvnc_transport_rfb_flow_t *flow,
    size_t *available_count);

int pstvnc_transport_rfb_flow_release(
    pstvnc_transport_rfb_flow_t *flow);

#endif
