/*
 * File synopsis:
 * Defines the shared PSTV transport wire-header representation, stable logical
 * channel identities, exact provisional Wire-establishment codecs, typed RFB
 * provider-terminal reporting, and exact MPEG generation-control wire codecs.
 * This file owns framing vocabulary only; it does not own sockets, dispatch,
 * queues, active-session policy, exact-generation lifecycle state, media
 * policy, or threading.
 *
 * Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md; docs/ledge/
 * LEDGE_AUDIT_A003_MPEG_GENERATION.md.
 */

#ifndef PSTVNC_TRANSPORT_PROTOCOL_H
#define PSTVNC_TRANSPORT_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define PSTVNC_TRANSPORT_MAGIC 0x50535456u
#define PSTVNC_TRANSPORT_VERSION 1u
#define PSTVNC_TRANSPORT_HEADER_SIZE 16u
#define PSTVNC_TRANSPORT_MAX_PAYLOAD 8192u
#define PSTVNC_TRANSPORT_CREDIT_PAYLOAD_SIZE 4u

/*
 * Product-establishment version 2 is the Q4 compatibility fence for the R16A
 * channel-1 ERROR semantic. Fixed header/Wire framing remains version 1.
 */
#define PSTVNC_WIRE_PRODUCT_ESTABLISHMENT_VERSION 2u
#define PSTVNC_WIRE_HELLO_PAYLOAD_SIZE 8u
#define PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE 4u
#define PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE 4u
#define PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE 4u

#define PSTVNC_MPEG_GENERATION_CONTROL_VERSION 1u
#define PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE 12u
#define PSTVNC_MPEG_START_PAYLOAD_SIZE 44u

typedef enum pstvnc_transport_frame_kind {
    PSTVNC_TRANSPORT_FRAME_HELLO = 1,
    PSTVNC_TRANSPORT_FRAME_CONFIG = 2,
    PSTVNC_TRANSPORT_FRAME_DATA = 3,
    PSTVNC_TRANSPORT_FRAME_CREDIT = 4,
    PSTVNC_TRANSPORT_FRAME_TELEMETRY = 5,
    PSTVNC_TRANSPORT_FRAME_HEARTBEAT = 6,
    PSTVNC_TRANSPORT_FRAME_ERROR = 7,
    PSTVNC_TRANSPORT_FRAME_MPEG_RETIRE = 10,
    PSTVNC_TRANSPORT_FRAME_MPEG_START = 11,
    PSTVNC_TRANSPORT_FRAME_ACCEPT = 12,
    PSTVNC_TRANSPORT_FRAME_NOT_ACCEPTED = 13
} pstvnc_transport_frame_kind_t;

typedef enum pstvnc_transport_channel {
    PSTVNC_TRANSPORT_CHANNEL_CONTROL = 0,
    PSTVNC_TRANSPORT_CHANNEL_RFB = 1,
    PSTVNC_TRANSPORT_CHANNEL_AUDIO = 2,
    PSTVNC_TRANSPORT_CHANNEL_TELEMETRY = 3,
    PSTVNC_TRANSPORT_CHANNEL_MPEG2 = 4
} pstvnc_transport_channel_t;

typedef struct pstvnc_transport_header {
    uint8_t version;
    uint8_t kind;
    uint8_t channel;
    uint8_t flags;
    uint32_t sequence;
    uint32_t payload_length;
} pstvnc_transport_header_t;

typedef enum pstvnc_wire_not_accepted_reason {
    PSTVNC_WIRE_NOT_ACCEPTED_WIRE_VERSION = 1,
    PSTVNC_WIRE_NOT_ACCEPTED_PRODUCT_VERSION = 2,
    PSTVNC_WIRE_NOT_ACCEPTED_MALFORMED = 3
} pstvnc_wire_not_accepted_reason_t;

typedef struct pstvnc_wire_hello_payload {
    uint32_t wire_version;
    uint32_t product_establishment_version;
} pstvnc_wire_hello_payload_t;

typedef struct pstvnc_wire_accept_payload {
    uint32_t session_id;
} pstvnc_wire_accept_payload_t;

typedef struct pstvnc_wire_not_accepted_payload {
    uint32_t reason;
} pstvnc_wire_not_accepted_payload_t;

/*
 * R16A assigns the previously dormant ERROR=7 reservation on channel 1 to this
 * exact one-word mechanism contract. READ covers provider EOF and recv/read
 * failure; all values are RFB-local facts and do not imply Wire failure.
 */
typedef enum pstvnc_rfb_provider_failure_reason {
    PSTVNC_RFB_PROVIDER_FAILURE_NONE = 0,
    PSTVNC_RFB_PROVIDER_FAILURE_CONNECT = 1,
    PSTVNC_RFB_PROVIDER_FAILURE_READ = 2,
    PSTVNC_RFB_PROVIDER_FAILURE_WRITE = 3
} pstvnc_rfb_provider_failure_reason_t;

typedef struct pstvnc_rfb_provider_failure_payload {
    uint32_t reason;
} pstvnc_rfb_provider_failure_payload_t;

typedef struct pstvnc_mpeg_retire_payload {
    uint32_t version;
    uint32_t session_id;
    uint32_t generation;
} pstvnc_mpeg_retire_payload_t;

typedef struct pstvnc_mpeg_start_payload {
    uint32_t version;
    uint32_t session_id;
    uint32_t generation;
    uint32_t base_x;
    uint32_t base_y;
    uint32_t base_width;
    uint32_t base_height;
    uint32_t suppression_x;
    uint32_t suppression_y;
    uint32_t suppression_width;
    uint32_t suppression_height;
} pstvnc_mpeg_start_payload_t;

int pstvnc_transport_header_encode(
    uint8_t output[PSTVNC_TRANSPORT_HEADER_SIZE],
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_decode(
    pstvnc_transport_header_t *header,
    const uint8_t input[PSTVNC_TRANSPORT_HEADER_SIZE]);
uint32_t pstvnc_transport_read_be32(const uint8_t input[4]);
void pstvnc_transport_write_be32(uint8_t output[4], uint32_t value);

int pstvnc_wire_hello_payload_encode(
    uint8_t output[PSTVNC_WIRE_HELLO_PAYLOAD_SIZE],
    const pstvnc_wire_hello_payload_t *payload);
int pstvnc_wire_hello_payload_decode(
    pstvnc_wire_hello_payload_t *payload,
    const uint8_t *input,
    size_t input_size);
int pstvnc_wire_accept_payload_encode(
    uint8_t output[PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE],
    const pstvnc_wire_accept_payload_t *payload);
int pstvnc_wire_accept_payload_decode(
    pstvnc_wire_accept_payload_t *payload,
    const uint8_t *input,
    size_t input_size);
int pstvnc_wire_not_accepted_payload_encode(
    uint8_t output[PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE],
    const pstvnc_wire_not_accepted_payload_t *payload);
int pstvnc_wire_not_accepted_payload_decode(
    pstvnc_wire_not_accepted_payload_t *payload,
    const uint8_t *input,
    size_t input_size);

int pstvnc_rfb_provider_failure_payload_encode(
    uint8_t output[PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE],
    const pstvnc_rfb_provider_failure_payload_t *payload);
int pstvnc_rfb_provider_failure_payload_decode(
    pstvnc_rfb_provider_failure_payload_t *payload,
    const uint8_t *input,
    size_t input_size);

int pstvnc_mpeg_retire_payload_encode(
    uint8_t output[PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE],
    const pstvnc_mpeg_retire_payload_t *payload);
int pstvnc_mpeg_retire_payload_decode(
    pstvnc_mpeg_retire_payload_t *payload,
    const uint8_t *input,
    size_t input_size);
int pstvnc_mpeg_start_payload_encode(
    uint8_t output[PSTVNC_MPEG_START_PAYLOAD_SIZE],
    const pstvnc_mpeg_start_payload_t *payload);
int pstvnc_mpeg_start_payload_decode(
    pstvnc_mpeg_start_payload_t *payload,
    const uint8_t *input,
    size_t input_size);

int pstvnc_transport_header_is_wire_hello(
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_is_wire_accept(
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_is_wire_not_accepted(
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_is_rfb_provider_failure(
    const pstvnc_transport_header_t *header);

int pstvnc_transport_header_is_mpeg_data(
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_is_mpeg_retire(
    const pstvnc_transport_header_t *header);
int pstvnc_transport_header_is_mpeg_start(
    const pstvnc_transport_header_t *header);

#endif
