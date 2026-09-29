#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "transport/protocol.h"

static int failures;

#define CHECK(expression)                                                \
    do {                                                                 \
        if (!(expression)) {                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n",                         \
                    __FILE__, __LINE__, #expression);                    \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void test_big_endian_helpers(void)
{
    uint8_t bytes[4];

    pstvnc_transport_write_be32(bytes, 0x12345678u);
    CHECK(bytes[0] == 0x12u);
    CHECK(bytes[1] == 0x34u);
    CHECK(bytes[2] == 0x56u);
    CHECK(bytes[3] == 0x78u);
    CHECK(pstvnc_transport_read_be32(bytes) == 0x12345678u);
}

static void test_header_round_trip_and_exact_wire_bytes(void)
{
    pstvnc_transport_header_t input;
    pstvnc_transport_header_t output;
    uint8_t wire[PSTVNC_TRANSPORT_HEADER_SIZE];
    static const uint8_t expected[PSTVNC_TRANSPORT_HEADER_SIZE] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x03u, 0x01u, 0xa5u,
        0x01u, 0x02u, 0x03u, 0x04u,
        0x00u, 0x00u, 0x10u, 0x00u
    };

    memset(&input, 0, sizeof(input));
    memset(&output, 0, sizeof(output));
    memset(wire, 0, sizeof(wire));

    input.version = PSTVNC_TRANSPORT_VERSION;
    input.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    input.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;
    input.flags = 0xa5u;
    input.sequence = 0x01020304u;
    input.payload_length = 4096u;

    CHECK(pstvnc_transport_header_encode(wire, &input));
    CHECK(memcmp(wire, expected, sizeof(wire)) == 0);
    CHECK(pstvnc_transport_header_decode(&output, wire));
    CHECK(output.version == input.version);
    CHECK(output.kind == input.kind);
    CHECK(output.channel == input.channel);
    CHECK(output.flags == input.flags);
    CHECK(output.sequence == input.sequence);
    CHECK(output.payload_length == input.payload_length);
}

static void test_encode_rejects_invalid_contract(void)
{
    pstvnc_transport_header_t header;
    uint8_t wire[PSTVNC_TRANSPORT_HEADER_SIZE];

    memset(&header, 0, sizeof(header));
    memset(wire, 0, sizeof(wire));

    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;

    CHECK(!pstvnc_transport_header_encode(NULL, &header));
    CHECK(!pstvnc_transport_header_encode(wire, NULL));

    header.version = (uint8_t)(PSTVNC_TRANSPORT_VERSION + 1u);
    CHECK(!pstvnc_transport_header_encode(wire, &header));

    header.version = PSTVNC_TRANSPORT_VERSION;
    header.payload_length = PSTVNC_TRANSPORT_MAX_PAYLOAD + 1u;
    CHECK(!pstvnc_transport_header_encode(wire, &header));
}

static void test_decode_rejects_invalid_wire_contract(void)
{
    pstvnc_transport_header_t header;
    uint8_t wire[PSTVNC_TRANSPORT_HEADER_SIZE];

    memset(&header, 0, sizeof(header));
    memset(wire, 0, sizeof(wire));

    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;
    header.sequence = 7u;
    header.payload_length = 1u;
    CHECK(pstvnc_transport_header_encode(wire, &header));

    CHECK(!pstvnc_transport_header_decode(NULL, wire));
    CHECK(!pstvnc_transport_header_decode(&header, NULL));

    wire[0] ^= 0xffu;
    CHECK(!pstvnc_transport_header_decode(&header, wire));
    wire[0] ^= 0xffu;

    wire[4] = (uint8_t)(PSTVNC_TRANSPORT_VERSION + 1u);
    CHECK(!pstvnc_transport_header_decode(&header, wire));
    wire[4] = PSTVNC_TRANSPORT_VERSION;

    pstvnc_transport_write_be32(
        &wire[12],
        PSTVNC_TRANSPORT_MAX_PAYLOAD + 1u);
    CHECK(!pstvnc_transport_header_decode(&header, wire));
}

static void test_wire_establishment_exact_codecs_and_frames(void)
{
    pstvnc_transport_header_t header;
    pstvnc_wire_hello_payload_t hello;
    pstvnc_wire_hello_payload_t decoded_hello;
    pstvnc_wire_accept_payload_t acceptance;
    pstvnc_wire_accept_payload_t decoded_acceptance;
    pstvnc_wire_not_accepted_payload_t rejection;
    pstvnc_wire_not_accepted_payload_t decoded_rejection;
    uint8_t header_bytes[PSTVNC_TRANSPORT_HEADER_SIZE];
    uint8_t payload[PSTVNC_WIRE_HELLO_PAYLOAD_SIZE];
    uint8_t frame[PSTVNC_TRANSPORT_HEADER_SIZE + PSTVNC_WIRE_HELLO_PAYLOAD_SIZE];
    static const uint8_t expected_hello[] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x01u,
        0x00u, 0x00u, 0x00u, 0x08u,
        0x00u, 0x00u, 0x00u, 0x01u,
        0x00u, 0x00u, 0x00u, 0x02u
    };
    static const uint8_t expected_accept[] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x0cu, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x01u,
        0x00u, 0x00u, 0x00u, 0x04u,
        0x12u, 0x34u, 0x56u, 0x78u
    };
    static const uint8_t expected_reject[] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x0du, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x01u,
        0x00u, 0x00u, 0x00u, 0x04u,
        0x00u, 0x00u, 0x00u, 0x02u
    };

    CHECK(PSTVNC_TRANSPORT_VERSION == 1u);
    CHECK(PSTVNC_WIRE_PRODUCT_ESTABLISHMENT_VERSION == 2u);

    memset(&header, 0, sizeof(header));
    hello.wire_version = PSTVNC_TRANSPORT_VERSION;
    hello.product_establishment_version =
        PSTVNC_WIRE_PRODUCT_ESTABLISHMENT_VERSION;
    CHECK(pstvnc_wire_hello_payload_encode(payload, &hello));

    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_HELLO;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_CONTROL;
    header.flags = 0u;
    header.sequence = 1u;
    header.payload_length = PSTVNC_WIRE_HELLO_PAYLOAD_SIZE;
    CHECK(pstvnc_transport_header_encode(header_bytes, &header));
    memcpy(frame, header_bytes, sizeof(header_bytes));
    memcpy(frame + sizeof(header_bytes), payload, PSTVNC_WIRE_HELLO_PAYLOAD_SIZE);
    CHECK(memcmp(frame, expected_hello, sizeof(expected_hello)) == 0);
    CHECK(pstvnc_transport_header_is_wire_hello(&header));
    CHECK(pstvnc_wire_hello_payload_decode(
        &decoded_hello, payload, PSTVNC_WIRE_HELLO_PAYLOAD_SIZE));
    CHECK(decoded_hello.wire_version == PSTVNC_TRANSPORT_VERSION);
    CHECK(decoded_hello.product_establishment_version ==
        PSTVNC_WIRE_PRODUCT_ESTABLISHMENT_VERSION);

    acceptance.session_id = 0x12345678u;
    CHECK(pstvnc_wire_accept_payload_encode(payload, &acceptance));
    header.kind = PSTVNC_TRANSPORT_FRAME_ACCEPT;
    header.payload_length = PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE;
    CHECK(pstvnc_transport_header_encode(header_bytes, &header));
    memcpy(frame, header_bytes, sizeof(header_bytes));
    memcpy(frame + sizeof(header_bytes), payload, PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE);
    CHECK(memcmp(frame, expected_accept, sizeof(expected_accept)) == 0);
    CHECK(pstvnc_transport_header_is_wire_accept(&header));
    CHECK(pstvnc_wire_accept_payload_decode(
        &decoded_acceptance, payload, PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE));
    CHECK(decoded_acceptance.session_id == 0x12345678u);

    acceptance.session_id = 0u;
    CHECK(!pstvnc_wire_accept_payload_encode(payload, &acceptance));
    memset(payload, 0, PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE);
    CHECK(!pstvnc_wire_accept_payload_decode(
        &decoded_acceptance, payload, PSTVNC_WIRE_ACCEPT_PAYLOAD_SIZE));

    rejection.reason = PSTVNC_WIRE_NOT_ACCEPTED_PRODUCT_VERSION;
    CHECK(pstvnc_wire_not_accepted_payload_encode(payload, &rejection));
    header.kind = PSTVNC_TRANSPORT_FRAME_NOT_ACCEPTED;
    header.payload_length = PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE;
    CHECK(pstvnc_transport_header_encode(header_bytes, &header));
    memcpy(frame, header_bytes, sizeof(header_bytes));
    memcpy(frame + sizeof(header_bytes), payload,
        PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE);
    CHECK(memcmp(frame, expected_reject, sizeof(expected_reject)) == 0);
    CHECK(pstvnc_transport_header_is_wire_not_accepted(&header));
    CHECK(pstvnc_wire_not_accepted_payload_decode(
        &decoded_rejection, payload, PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE));
    CHECK(decoded_rejection.reason ==
        (uint32_t)PSTVNC_WIRE_NOT_ACCEPTED_PRODUCT_VERSION);

    rejection.reason = 0u;
    CHECK(!pstvnc_wire_not_accepted_payload_encode(payload, &rejection));
    rejection.reason = 4u;
    CHECK(!pstvnc_wire_not_accepted_payload_encode(payload, &rejection));
    pstvnc_transport_write_be32(payload, 4u);
    CHECK(!pstvnc_wire_not_accepted_payload_decode(
        &decoded_rejection, payload, PSTVNC_WIRE_NOT_ACCEPTED_PAYLOAD_SIZE));

    header.kind = PSTVNC_TRANSPORT_FRAME_HELLO;
    header.payload_length = PSTVNC_WIRE_HELLO_PAYLOAD_SIZE;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;
    CHECK(!pstvnc_transport_header_is_wire_hello(&header));
    header.channel = PSTVNC_TRANSPORT_CHANNEL_CONTROL;
    header.flags = 1u;
    CHECK(!pstvnc_transport_header_is_wire_hello(&header));
}

static void test_rfb_provider_failure_exact_codec_and_identity(void)
{
    pstvnc_rfb_provider_failure_payload_t input;
    pstvnc_rfb_provider_failure_payload_t output;
    pstvnc_transport_header_t header;
    uint8_t payload[PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE];
    static const pstvnc_rfb_provider_failure_reason_t reasons[] = {
        PSTVNC_RFB_PROVIDER_FAILURE_CONNECT,
        PSTVNC_RFB_PROVIDER_FAILURE_READ,
        PSTVNC_RFB_PROVIDER_FAILURE_WRITE
    };
    size_t index;

    for (index = 0u; index < sizeof(reasons) / sizeof(reasons[0]); index++) {
        input.reason = (uint32_t)reasons[index];
        memset(&output, 0, sizeof(output));
        CHECK(pstvnc_rfb_provider_failure_payload_encode(payload, &input));
        CHECK(pstvnc_transport_read_be32(payload) == input.reason);
        CHECK(pstvnc_rfb_provider_failure_payload_decode(
            &output, payload, sizeof(payload)));
        CHECK(output.reason == input.reason);
    }

    input.reason = (uint32_t)PSTVNC_RFB_PROVIDER_FAILURE_NONE;
    CHECK(!pstvnc_rfb_provider_failure_payload_encode(payload, &input));
    input.reason = 4u;
    CHECK(!pstvnc_rfb_provider_failure_payload_encode(payload, &input));
    pstvnc_transport_write_be32(payload, 0u);
    CHECK(!pstvnc_rfb_provider_failure_payload_decode(
        &output, payload, sizeof(payload)));
    pstvnc_transport_write_be32(payload, 4u);
    CHECK(!pstvnc_rfb_provider_failure_payload_decode(
        &output, payload, sizeof(payload)));

    memset(&header, 0, sizeof(header));
    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_ERROR;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;
    header.flags = 0u;
    header.sequence = 2u;
    header.payload_length = PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE;
    CHECK(pstvnc_transport_header_is_rfb_provider_failure(&header));

    header.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    header.payload_length = 0u;
    CHECK(!pstvnc_transport_header_is_rfb_provider_failure(&header));
    CHECK(header.channel == PSTVNC_TRANSPORT_CHANNEL_RFB);
    CHECK(header.payload_length == 0u);

    header.kind = PSTVNC_TRANSPORT_FRAME_ERROR;
    header.payload_length = PSTVNC_RFB_PROVIDER_FAILURE_PAYLOAD_SIZE;
    header.flags = 1u;
    CHECK(!pstvnc_transport_header_is_rfb_provider_failure(&header));
    header.flags = 0u;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_CONTROL;
    CHECK(!pstvnc_transport_header_is_rfb_provider_failure(&header));
}

static void test_rfb_data_credit_exact_wire_bytes(void)
{
    pstvnc_transport_header_t header;
    uint8_t header_bytes[PSTVNC_TRANSPORT_HEADER_SIZE];
    uint8_t credit_payload[PSTVNC_TRANSPORT_CREDIT_PAYLOAD_SIZE];
    uint8_t credit_frame[
        PSTVNC_TRANSPORT_HEADER_SIZE + PSTVNC_TRANSPORT_CREDIT_PAYLOAD_SIZE];
    uint8_t data_frame[PSTVNC_TRANSPORT_HEADER_SIZE + 3u];
    static const uint8_t expected_credit[] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x04u, 0x01u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x02u,
        0x00u, 0x00u, 0x00u, 0x04u,
        0x00u, 0x00u, 0x00u, 0x08u
    };
    static const uint8_t expected_data[] = {
        0x50u, 0x53u, 0x54u, 0x56u,
        0x01u, 0x03u, 0x01u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u,
        0x00u, 0x00u, 0x00u, 0x03u,
        0x61u, 0x62u, 0x63u
    };

    memset(&header, 0, sizeof(header));
    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_CREDIT;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_RFB;
    header.flags = 0u;
    header.sequence = 2u;
    header.payload_length = PSTVNC_TRANSPORT_CREDIT_PAYLOAD_SIZE;
    pstvnc_transport_write_be32(credit_payload, 8u);

    CHECK(pstvnc_transport_header_encode(header_bytes, &header));
    memcpy(credit_frame, header_bytes, sizeof(header_bytes));
    memcpy(
        credit_frame + sizeof(header_bytes),
        credit_payload,
        sizeof(credit_payload));
    CHECK(memcmp(
        credit_frame,
        expected_credit,
        sizeof(expected_credit)) == 0);

    header.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    header.sequence = 3u;
    header.payload_length = 3u;
    CHECK(pstvnc_transport_header_encode(header_bytes, &header));
    memcpy(data_frame, header_bytes, sizeof(header_bytes));
    memcpy(data_frame + sizeof(header_bytes), "abc", 3u);
    CHECK(memcmp(data_frame, expected_data, sizeof(expected_data)) == 0);
}

static void test_mpeg_retire_exact_codec(void)
{
    pstvnc_mpeg_retire_payload_t input;
    pstvnc_mpeg_retire_payload_t output;
    uint8_t wire[PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE];
    static const uint8_t expected[PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE] = {
        0x00u, 0x00u, 0x00u, 0x01u,
        0x12u, 0x34u, 0x56u, 0x78u,
        0x9au, 0xbcu, 0xdeu, 0xf0u
    };

    input.version = PSTVNC_MPEG_GENERATION_CONTROL_VERSION;
    input.session_id = 0x12345678u;
    input.generation = 0x9abcdef0u;
    memset(&output, 0, sizeof(output));

    CHECK(pstvnc_mpeg_retire_payload_encode(wire, &input));
    CHECK(memcmp(wire, expected, sizeof(wire)) == 0);
    CHECK(pstvnc_mpeg_retire_payload_decode(&output, wire, sizeof(wire)));
    CHECK(output.version == input.version);
    CHECK(output.session_id == input.session_id);
    CHECK(output.generation == input.generation);
    CHECK(!pstvnc_mpeg_retire_payload_decode(&output, wire, sizeof(wire) - 1u));

    wire[3] = 2u;
    CHECK(!pstvnc_mpeg_retire_payload_decode(&output, wire, sizeof(wire)));
}

static void fill_start(pstvnc_mpeg_start_payload_t *start)
{
    memset(start, 0, sizeof(*start));
    start->version = PSTVNC_MPEG_GENERATION_CONTROL_VERSION;
    start->session_id = 0x10203040u;
    start->generation = 7u;
    start->base_x = 0u;
    start->base_y = 0u;
    start->base_width = 640u;
    start->base_height = 448u;
    start->suppression_x = 16u;
    start->suppression_y = 24u;
    start->suppression_width = 320u;
    start->suppression_height = 180u;
}

static void test_mpeg_start_exact_codec(void)
{
    pstvnc_mpeg_start_payload_t input;
    pstvnc_mpeg_start_payload_t output;
    uint8_t wire[PSTVNC_MPEG_START_PAYLOAD_SIZE];
    uint8_t rewritten[PSTVNC_MPEG_START_PAYLOAD_SIZE];

    fill_start(&input);
    memset(&output, 0, sizeof(output));

    CHECK(pstvnc_mpeg_start_payload_encode(wire, &input));
    CHECK(pstvnc_mpeg_start_payload_decode(&output, wire, sizeof(wire)));
    CHECK(memcmp(&output, &input, sizeof(input)) == 0);
    CHECK(pstvnc_mpeg_start_payload_encode(rewritten, &output));
    CHECK(memcmp(rewritten, wire, sizeof(wire)) == 0);
    CHECK(!pstvnc_mpeg_start_payload_decode(&output, wire, sizeof(wire) - 1u));

    wire[3] = 2u;
    CHECK(!pstvnc_mpeg_start_payload_decode(&output, wire, sizeof(wire)));
}

static void test_explicit_mpeg_frame_identity_ignores_payload_shape(void)
{
    pstvnc_transport_header_t header;
    pstvnc_mpeg_start_payload_t start;
    uint8_t collision_payload[PSTVNC_MPEG_START_PAYLOAD_SIZE];

    fill_start(&start);
    CHECK(pstvnc_mpeg_start_payload_encode(collision_payload, &start));

    memset(&header, 0, sizeof(header));
    header.version = PSTVNC_TRANSPORT_VERSION;
    header.kind = PSTVNC_TRANSPORT_FRAME_DATA;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_MPEG2;
    header.flags = 0u;
    header.payload_length = sizeof(collision_payload);

    CHECK(pstvnc_transport_header_is_mpeg_data(&header));
    CHECK(!pstvnc_transport_header_is_mpeg_start(&header));
    CHECK(!pstvnc_transport_header_is_mpeg_retire(&header));

    header.kind = PSTVNC_TRANSPORT_FRAME_MPEG_START;
    header.channel = PSTVNC_TRANSPORT_CHANNEL_CONTROL;
    CHECK(!pstvnc_transport_header_is_mpeg_data(&header));
    CHECK(pstvnc_transport_header_is_mpeg_start(&header));

    header.kind = PSTVNC_TRANSPORT_FRAME_MPEG_RETIRE;
    header.payload_length = PSTVNC_MPEG_RETIRE_PAYLOAD_SIZE;
    CHECK(!pstvnc_transport_header_is_mpeg_data(&header));
    CHECK(!pstvnc_transport_header_is_mpeg_start(&header));
    CHECK(pstvnc_transport_header_is_mpeg_retire(&header));
}

int main(void)
{
    test_big_endian_helpers();
    test_header_round_trip_and_exact_wire_bytes();
    test_encode_rejects_invalid_contract();
    test_decode_rejects_invalid_wire_contract();
    test_wire_establishment_exact_codecs_and_frames();
    test_rfb_provider_failure_exact_codec_and_identity();
    test_rfb_data_credit_exact_wire_bytes();
    test_mpeg_retire_exact_codec();
    test_mpeg_start_exact_codec();
    test_explicit_mpeg_frame_identity_ignores_payload_shape();

    if (failures != 0) {
        fprintf(stderr, "transport protocol tests failed: %d\n", failures);
        return 1;
    }

    puts("transport protocol tests passed");
    return 0;
}
