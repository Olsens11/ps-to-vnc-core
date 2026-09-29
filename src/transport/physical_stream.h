/*
 * File synopsis:
 * Defines the transport-internal owner for the one adopted physical PSTV
 * socket and its serialized framed-send and sole-receive sequencing state.
 * It also owns the Transport-internal Q4 client establishment transaction that
 * advances one adopted physical lineage from sequence 1 to sequence 2 before
 * any rider runtime can inherit it. This boundary owns physical framing only;
 * it does not own logical-channel queues, RFB semantics, exact-generation
 * lifecycle state, media policy, or application lifecycle policy.
 *
 * Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md; docs/ledge/
 * LEDGE_AUDIT_A003_MPEG_GENERATION.md.
 */

#ifndef PSTVNC_TRANSPORT_PHYSICAL_STREAM_H
#define PSTVNC_TRANSPORT_PHYSICAL_STREAM_H

#include "protocol.h"

#include <stddef.h>
#include <stdint.h>

typedef struct pstvnc_transport_physical_stream {
    int socket_fd;
    int send_semaphore_id;
    uint32_t next_send_sequence;
    uint32_t expected_receive_sequence;
} pstvnc_transport_physical_stream_t;

/* Ownership of socket_fd transfers to stream only when this call succeeds. */
int pstvnc_transport_physical_stream_adopt(
    pstvnc_transport_physical_stream_t *stream,
    int socket_fd);

/*
 * Adopt one caller-owned TCP descriptor and perform exact product Q4 as the
 * sole physical owner. Once descriptor adoption succeeds, *socket_fd becomes
 * -1 even when establishment later fails: Transport owns the close from that
 * point forward. Return 1 for ACCEPT, -1 for exact NOT_ACCEPTED, and 0 for
 * framing/I/O/mechanism failure. ACCEPT leaves the stream at sequence 2/2.
 */
int pstvnc_transport_physical_stream_establish_client(
    pstvnc_transport_physical_stream_t *stream,
    int *socket_fd,
    uint32_t *session_id,
    pstvnc_wire_not_accepted_reason_t *rejection_reason);

/*
 * Move an already-established sequence-2 physical lineage without exposing a
 * public sequence seed. The source becomes inert only on success.
 */
int pstvnc_transport_physical_stream_transfer_established(
    pstvnc_transport_physical_stream_t *destination,
    pstvnc_transport_physical_stream_t *source);

/*
 * Sends one complete PSTV frame under the stream's single send lock.
 * Sequence advances only after the entire header and payload are committed.
 */
int pstvnc_transport_physical_stream_send_frame(
    pstvnc_transport_physical_stream_t *stream,
    uint8_t kind,
    uint8_t channel,
    uint8_t flags,
    const void *payload,
    size_t payload_length);

/*
 * Receives exactly one complete PSTV frame from the sole physical socket.
 * The caller supplies storage large enough for the accepted payload ceiling.
 * Inbound sequence advances only after the complete payload has been read.
 */
/*
 * Wait for inbound socket readability without consuming Wire bytes.
 * Returns 1 when readable, 0 on timeout, and -1 on readiness failure.
 */
int pstvnc_transport_physical_stream_wait_readable(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t timeout_us);

int pstvnc_transport_physical_stream_receive_frame(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_header_t *header,
    void *payload,
    size_t payload_capacity);

/*
 * Interrupt current-session physical I/O without transferring or releasing
 * descriptor ownership. Transport uses this only to make a blocked sole
 * receiver converge after an application-local fatal-session request.
 */
int pstvnc_transport_physical_stream_shutdown_io(
    pstvnc_transport_physical_stream_t *stream);

/*
 * Releases the adopted socket and send lock. The higher transport runtime must
 * prove receiver/dispatch quiescence before calling this once receive exists.
 */
void pstvnc_transport_physical_stream_release(
    pstvnc_transport_physical_stream_t *stream);

#endif