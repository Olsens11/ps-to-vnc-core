/*
 * Hardware checkpoint 03B-DIAG: post-proof lifecycle stage telemetry.
 *
 * Main thread remains the sole physical Wire owner. A second EE thread behaves
 * like a parser-side consumer and blocks in the extracted rfb_flow activity
 * wait. Credit, provider-terminal, full runtime, parser, and display are absent.
 */
#include <kernel.h>
#include <ps2ip.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "platform/ps2_network.h"
#include "platform/ps2_system.h"
#include "transport/physical_stream.h"
#include "transport/protocol.h"
#include "transport/rfb_flow.h"

#define RFB_PAYLOAD_BYTES 328u
#define PRE_BLOCKED_ROUNDS 128u
#define POST_BLOCKED_ROUNDS 128u
#define TOTAL_FRAMES (1u + PRE_BLOCKED_ROUNDS + 1u + POST_BLOCKED_ROUNDS)
#define QUEUE_CAPACITY 1024u
#define READINESS_TIMEOUT_US 1000u
#define EMPTY_POLL_YIELD_US 1000u
#define ACTIVE_POLL_LIMIT 5000u
#define IDLE_POLL_LIMIT 70000u
#define ARM_POLL_LIMIT 5000u
#define CONSUMER_STACK_BYTES 8192u
#define CONSUMER_PRIORITY 64

#define CONTROL_READY 1u
#define CONTROL_IDLE_READY 2u
#define CONTROL_IDLE_RESULT 3u
#define CONTROL_PASS 4u

#define TELEMETRY_MAGIC 0x52334447u
#define TELEMETRY_PORT 5999u

#define STAGE_DIAG_READY 1u
#define STAGE_CONSUMER_DONE_WAIT_ENTER 10u
#define STAGE_CONSUMER_DONE_WAIT_RETURN 11u
#define STAGE_ACTIVITY_SNAPSHOT_ENTER 12u
#define STAGE_ACTIVITY_SNAPSHOT_RETURN 13u
#define STAGE_AVAILABLE_ENTER 14u
#define STAGE_AVAILABLE_RETURN 15u
#define STAGE_PASS_SEND_ENTER 16u
#define STAGE_PASS_SEND_RETURN 17u
#define STAGE_DONE_WAIT_ENTER 18u
#define STAGE_DONE_WAIT_RETURN 19u
#define STAGE_DORMANT_WAIT_ENTER 20u
#define STAGE_DORMANT_WAIT_RETURN 21u
#define STAGE_DELETE_THREAD_ENTER 22u
#define STAGE_DELETE_THREAD_RETURN 23u
#define STAGE_DELETE_DONE_SEMA_ENTER 24u
#define STAGE_DELETE_DONE_SEMA_RETURN 25u
#define STAGE_DELETE_PROGRESS_SEMA_ENTER 26u
#define STAGE_DELETE_PROGRESS_SEMA_RETURN 27u
#define STAGE_DELETE_START_SEMA_ENTER 28u
#define STAGE_DELETE_START_SEMA_RETURN 29u
#define STAGE_DELETE_SNAPSHOT_SEMA_ENTER 30u
#define STAGE_DELETE_SNAPSHOT_SEMA_RETURN 31u
#define STAGE_FLOW_RELEASE_ENTER 32u
#define STAGE_FLOW_RELEASE_RETURN 33u
#define STAGE_PHYSICAL_RELEASE_ENTER 34u
#define STAGE_PHYSICAL_RELEASE_RETURN 35u
#define STAGE_NETWORK_CLOSE_ENTER 36u
#define STAGE_NETWORK_CLOSE_RETURN 37u
#define STAGE_OSDSYS_ENTER 38u
#define STAGE_FAILURE_PATH_ENTER 90u

typedef struct consumer_context {
    pstvnc_transport_rfb_flow_t *flow;
    int snapshot_ready_semaphore_id;
    int start_wait_semaphore_id;
    int progress_semaphore_id;
    int done_semaphore_id;
    volatile int failed;
} consumer_context_t;

static uint8_t queue_storage[QUEUE_CAPACITY];
static uint8_t physical_payload[PSTVNC_TRANSPORT_MAX_PAYLOAD];
static uint8_t consumer_payload[RFB_PAYLOAD_BYTES];
static unsigned char consumer_stack[CONSUMER_STACK_BYTES]
    __attribute__((aligned(16)));

static int telemetry_socket = -1;
static struct sockaddr_in telemetry_address;
static uint32_t telemetry_sequence;

static void telemetry_init(void)
{
    telemetry_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (telemetry_socket < 0)
        return;

    memset(&telemetry_address, 0, sizeof(telemetry_address));
    telemetry_address.sin_len = sizeof(telemetry_address);
    telemetry_address.sin_family = AF_INET;
    telemetry_address.sin_port = htons(TELEMETRY_PORT);
    telemetry_address.sin_addr.s_addr =
        inet_addr(PSTVNC_PS2_GATEWAY_IP);
}

static void telemetry_emit(
    uint32_t stage,
    uint32_t value1,
    uint32_t value2)
{
    uint8_t payload[20];

    if (telemetry_socket < 0)
        return;

    telemetry_sequence++;
    pstvnc_transport_write_be32(&payload[0], TELEMETRY_MAGIC);
    pstvnc_transport_write_be32(&payload[4], telemetry_sequence);
    pstvnc_transport_write_be32(&payload[8], stage);
    pstvnc_transport_write_be32(&payload[12], value1);
    pstvnc_transport_write_be32(&payload[16], value2);

    (void)sendto(
        telemetry_socket,
        payload,
        sizeof(payload),
        0,
        (struct sockaddr *)&telemetry_address,
        sizeof(telemetry_address));
}

static int create_semaphore(int initial_count, int maximum_count)
{
    ee_sema_t semaphore;

    memset(&semaphore, 0, sizeof(semaphore));
    semaphore.init_count = initial_count;
    semaphore.max_count = maximum_count;
    semaphore.option = 0;
    return CreateSema(&semaphore);
}

static uint8_t expected_byte(uint32_t frame_index, uint32_t byte_index)
{
    return (uint8_t)(
        ((frame_index * 17u) ^
         (byte_index * 7u) ^
         0x5au) & 0xffu);
}

static int validate_consumer_payload(uint32_t frame_index)
{
    uint32_t index;

    for (index = 0u; index < RFB_PAYLOAD_BYTES; index++) {
        if (consumer_payload[index] != expected_byte(frame_index, index))
            return 0;
    }

    return 1;
}

static void consumer_thread(void *argument)
{
    consumer_context_t *context = (consumer_context_t *)argument;
    uint32_t sequence = 0u;
    uint32_t frame_index;

    if (!pstvnc_transport_rfb_flow_activity_snapshot(
            context->flow,
            &sequence)) {
        context->failed = 1;
        (void)SignalSema(context->done_semaphore_id);
        ExitThread();
    }

    if (SignalSema(context->snapshot_ready_semaphore_id) < 0 ||
        WaitSema(context->start_wait_semaphore_id) < 0) {
        context->failed = 1;
        (void)SignalSema(context->done_semaphore_id);
        ExitThread();
    }

    for (frame_index = 0u; frame_index < TOTAL_FRAMES; frame_index++) {
        if (!pstvnc_transport_rfb_flow_wait_activity(
                context->flow,
                &sequence) ||
            !pstvnc_transport_rfb_flow_read_exact(
                context->flow,
                consumer_payload,
                sizeof(consumer_payload)) ||
            !validate_consumer_payload(frame_index) ||
            SignalSema(context->progress_semaphore_id) < 0) {
            context->failed = 1;
            (void)SignalSema(context->done_semaphore_id);
            ExitThread();
        }
    }

    if (SignalSema(context->done_semaphore_id) < 0)
        context->failed = 1;

    ExitThread();
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
        physical_payload,
        sizeof(physical_payload));
}

static int receive_rfb_and_commit(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_rfb_flow_t *flow,
    uint32_t poll_limit,
    uint32_t *empty_polls)
{
    pstvnc_transport_header_t header;

    if (!receive_frame(stream, &header, poll_limit, empty_polls))
        return 0;

    if (header.kind != PSTVNC_TRANSPORT_FRAME_DATA ||
        header.channel != PSTVNC_TRANSPORT_CHANNEL_RFB ||
        header.flags != 0u ||
        header.payload_length != RFB_PAYLOAD_BYTES)
        return 0;

    return pstvnc_transport_rfb_flow_commit_data(
        flow,
        physical_payload,
        header.payload_length);
}

static int wait_activity_armed(
    pstvnc_transport_rfb_flow_t *flow)
{
    uint32_t polls;

    for (polls = 0u; polls < ARM_POLL_LIMIT; polls++) {
        int armed;

        if (WaitSema(flow->queue_semaphore_id) < 0)
            return 0;

        armed = flow->activity_wait_armed;

        if (SignalSema(flow->queue_semaphore_id) < 0)
            return 0;

        if (armed)
            return 1;

        if (pstvnc_ps2_system_delay_us(1000u) < 0)
            return 0;
    }

    return 0;
}

static int send_control(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t type,
    uint32_t value)
{
    uint8_t payload[12];

    pstvnc_transport_write_be32(&payload[0], 0x52303342u);
    pstvnc_transport_write_be32(&payload[4], type);
    pstvnc_transport_write_be32(&payload[8], value);

    return pstvnc_transport_physical_stream_send_frame(
        stream,
        PSTVNC_TRANSPORT_FRAME_DATA,
        PSTVNC_TRANSPORT_CHANNEL_CONTROL,
        0u,
        payload,
        sizeof(payload));
}

static int receive_control(
    pstvnc_transport_physical_stream_t *stream,
    uint32_t expected_type,
    uint32_t expected_value)
{
    pstvnc_transport_header_t header;

    if (!receive_frame(stream, &header, ACTIVE_POLL_LIMIT, NULL))
        return 0;

    if (header.kind != PSTVNC_TRANSPORT_FRAME_DATA ||
        header.channel != PSTVNC_TRANSPORT_CHANNEL_CONTROL ||
        header.flags != 0u ||
        header.payload_length != 12u)
        return 0;

    return pstvnc_transport_read_be32(&physical_payload[0]) == 0x52303342u &&
        pstvnc_transport_read_be32(&physical_payload[4]) == expected_type &&
        pstvnc_transport_read_be32(&physical_payload[8]) == expected_value;
}

static int wait_consumer_progress(consumer_context_t *context)
{
    if (WaitSema(context->progress_semaphore_id) < 0)
        return 0;

    return !context->failed;
}

static int run_blocked_round(
    pstvnc_transport_physical_stream_t *stream,
    pstvnc_transport_rfb_flow_t *flow,
    consumer_context_t *context,
    uint32_t frame_index)
{
    if (!wait_activity_armed(flow))
        return 0;

    if (!send_control(stream, CONTROL_READY, frame_index))
        return 0;

    if (!receive_rfb_and_commit(
            stream,
            flow,
            ACTIVE_POLL_LIMIT,
            NULL))
        return 0;

    return wait_consumer_progress(context);
}

static int start_consumer_thread(
    consumer_context_t *context,
    int *thread_id)
{
    ee_thread_t thread;

    if (context == NULL || thread_id == NULL)
        return 0;

    memset(&thread, 0, sizeof(thread));
    thread.func = (void *)consumer_thread;
    thread.stack = consumer_stack;
    thread.stack_size = (int)sizeof(consumer_stack);
    thread.gp_reg = &_gp;
    thread.initial_priority = CONSUMER_PRIORITY;
    thread.attr = 0;
    thread.option = 0;

    *thread_id = CreateThread(&thread);
    if (*thread_id < 0)
        return 0;

    if (StartThread(*thread_id, context) < 0) {
        (void)DeleteThread(*thread_id);
        *thread_id = -1;
        return 0;
    }

    return 1;
}

static int wait_consumer_dormant(int thread_id)
{
    uint32_t polls;

    for (polls = 0u; polls < 5000u; polls++) {
        ee_thread_status_t status;

        memset(&status, 0, sizeof(status));
        if (ReferThreadStatus(thread_id, &status) < 0)
            return 0;

        if (status.status == THS_DORMANT)
            return 1;

        if (pstvnc_ps2_system_delay_us(1000u) < 0)
            return 0;
    }

    return 0;
}

int main(int argc, char **argv)
{
    pstvnc_transport_physical_stream_t stream;
    pstvnc_transport_rfb_flow_t flow;
    pstvnc_wire_not_accepted_reason_t rejection_reason;
    consumer_context_t consumer;
    uint32_t session_id = 0u;
    uint32_t frame_index;
    uint32_t idle_empty_polls = 0u;
    uint32_t final_activity_sequence = 0u;
    size_t final_available = 0u;
    int socket_fd = -1;
    int stream_live = 0;
    int flow_live = 0;
    int consumer_thread_id = -1;
    int stage_result = 0;
    int success = 0;

    (void)argc;
    (void)argv;

    memset(&stream, 0, sizeof(stream));
    memset(&flow, 0, sizeof(flow));
    memset(&consumer, 0, sizeof(consumer));
    consumer.snapshot_ready_semaphore_id = -1;
    consumer.start_wait_semaphore_id = -1;
    consumer.progress_semaphore_id = -1;
    consumer.done_semaphore_id = -1;

    if (pstvnc_ps2_system_prepare_iop() < 0)
        goto done;
    if (pstvnc_ps2_network_init() < 0)
        goto done;
    if (pstvnc_ps2_network_wait_link() < 0)
        goto done;

    telemetry_init();
    telemetry_emit(STAGE_DIAG_READY, 0u, 0u);

    socket_fd = pstvnc_ps2_network_connect_management();
    if (socket_fd < 0)
        goto done;

    rejection_reason = (pstvnc_wire_not_accepted_reason_t)0;
    if (pstvnc_transport_physical_stream_establish_client(
            &stream,
            &socket_fd,
            &session_id,
            &rejection_reason) != 1 ||
        session_id == 0u)
        goto done;

    stream_live = 1;

    if (!pstvnc_transport_rfb_flow_initialize(
            &flow,
            queue_storage,
            sizeof(queue_storage)))
        goto done;

    flow_live = 1;
    consumer.flow = &flow;

    consumer.snapshot_ready_semaphore_id = create_semaphore(0, 1);
    consumer.start_wait_semaphore_id = create_semaphore(0, 1);
    consumer.progress_semaphore_id = create_semaphore(0, 1);
    consumer.done_semaphore_id = create_semaphore(0, 1);

    if (consumer.snapshot_ready_semaphore_id < 0 ||
        consumer.start_wait_semaphore_id < 0 ||
        consumer.progress_semaphore_id < 0 ||
        consumer.done_semaphore_id < 0)
        goto done;

    if (!start_consumer_thread(&consumer, &consumer_thread_id))
        goto done;

    /*
     * Lost-wake discriminator: the consumer snapshots sequence 0 and pauses
     * before entering wait_activity(). Frame 0 is then committed. Releasing the
     * gate must make wait_activity() observe the already-published sequence and
     * return without blocking.
     */
    if (WaitSema(consumer.snapshot_ready_semaphore_id) < 0)
        goto done;

    if (!receive_rfb_and_commit(
            &stream,
            &flow,
            ACTIVE_POLL_LIMIT,
            NULL))
        goto done;

    if (SignalSema(consumer.start_wait_semaphore_id) < 0 ||
        !wait_consumer_progress(&consumer))
        goto done;

    for (frame_index = 1u;
         frame_index <= PRE_BLOCKED_ROUNDS;
         frame_index++) {
        if (!run_blocked_round(
                &stream,
                &flow,
                &consumer,
                frame_index))
            goto done;
    }

    frame_index = 1u + PRE_BLOCKED_ROUNDS;

    if (!wait_activity_armed(&flow))
        goto done;

    if (!send_control(&stream, CONTROL_IDLE_READY, frame_index))
        goto done;

    if (!receive_rfb_and_commit(
            &stream,
            &flow,
            IDLE_POLL_LIMIT,
            &idle_empty_polls) ||
        !wait_consumer_progress(&consumer))
        goto done;

    if (!send_control(
            &stream,
            CONTROL_IDLE_RESULT,
            idle_empty_polls))
        goto done;

    for (frame_index++;
         frame_index < TOTAL_FRAMES;
         frame_index++) {
        if (!run_blocked_round(
                &stream,
                &flow,
                &consumer,
                frame_index))
            goto done;
    }

    telemetry_emit(
        STAGE_CONSUMER_DONE_WAIT_ENTER,
        (uint32_t)consumer.done_semaphore_id,
        0u);
    stage_result = WaitSema(consumer.done_semaphore_id) >= 0;
    telemetry_emit(
        STAGE_CONSUMER_DONE_WAIT_RETURN,
        (uint32_t)stage_result,
        (uint32_t)consumer.failed);
    if (!stage_result || consumer.failed || flow.failed)
        goto done;

    telemetry_emit(
        STAGE_ACTIVITY_SNAPSHOT_ENTER,
        final_activity_sequence,
        0u);
    stage_result = pstvnc_transport_rfb_flow_activity_snapshot(
        &flow,
        &final_activity_sequence);
    telemetry_emit(
        STAGE_ACTIVITY_SNAPSHOT_RETURN,
        (uint32_t)stage_result,
        final_activity_sequence);
    if (!stage_result)
        goto done;

    telemetry_emit(
        STAGE_AVAILABLE_ENTER,
        (uint32_t)final_available,
        0u);
    stage_result = pstvnc_transport_rfb_flow_available(
        &flow,
        &final_available);
    telemetry_emit(
        STAGE_AVAILABLE_RETURN,
        (uint32_t)stage_result,
        (uint32_t)final_available);
    if (!stage_result)
        goto done;

    if (final_activity_sequence != TOTAL_FRAMES ||
        final_available != 0u)
        goto done;

    telemetry_emit(
        STAGE_PASS_SEND_ENTER,
        final_activity_sequence,
        (uint32_t)final_available);
    stage_result = send_control(
        &stream,
        CONTROL_PASS,
        final_activity_sequence);
    telemetry_emit(
        STAGE_PASS_SEND_RETURN,
        (uint32_t)stage_result,
        stream.next_send_sequence);
    if (!stage_result)
        goto done;

    telemetry_emit(
        STAGE_DONE_WAIT_ENTER,
        stream.expected_receive_sequence,
        0u);
    stage_result = receive_control(
        &stream,
        5u,
        TOTAL_FRAMES);
    telemetry_emit(
        STAGE_DONE_WAIT_RETURN,
        (uint32_t)stage_result,
        stream.expected_receive_sequence);
    if (!stage_result)
        goto done;

    telemetry_emit(
        STAGE_DORMANT_WAIT_ENTER,
        (uint32_t)consumer_thread_id,
        0u);
    stage_result = wait_consumer_dormant(consumer_thread_id);
    telemetry_emit(
        STAGE_DORMANT_WAIT_RETURN,
        (uint32_t)stage_result,
        (uint32_t)consumer_thread_id);
    if (!stage_result)
        goto done;

    telemetry_emit(
        STAGE_DELETE_THREAD_ENTER,
        (uint32_t)consumer_thread_id,
        0u);
    stage_result = DeleteThread(consumer_thread_id) >= 0;
    telemetry_emit(
        STAGE_DELETE_THREAD_RETURN,
        (uint32_t)stage_result,
        (uint32_t)consumer_thread_id);
    if (!stage_result)
        goto done;

    consumer_thread_id = -1;
    success = 1;

done:
    if (!success)
        telemetry_emit(
            STAGE_FAILURE_PATH_ENTER,
            (uint32_t)consumer_thread_id,
            (uint32_t)flow.failed);

    if (consumer_thread_id >= 0)
        (void)TerminateThread(consumer_thread_id);

    telemetry_emit(
        STAGE_DELETE_DONE_SEMA_ENTER,
        (uint32_t)consumer.done_semaphore_id,
        0u);
    if (consumer.done_semaphore_id >= 0) {
        stage_result = DeleteSema(consumer.done_semaphore_id) >= 0;
        consumer.done_semaphore_id = -1;
    } else {
        stage_result = 1;
    }
    telemetry_emit(
        STAGE_DELETE_DONE_SEMA_RETURN,
        (uint32_t)stage_result,
        0u);

    telemetry_emit(
        STAGE_DELETE_PROGRESS_SEMA_ENTER,
        (uint32_t)consumer.progress_semaphore_id,
        0u);
    if (consumer.progress_semaphore_id >= 0) {
        stage_result = DeleteSema(consumer.progress_semaphore_id) >= 0;
        consumer.progress_semaphore_id = -1;
    } else {
        stage_result = 1;
    }
    telemetry_emit(
        STAGE_DELETE_PROGRESS_SEMA_RETURN,
        (uint32_t)stage_result,
        0u);

    telemetry_emit(
        STAGE_DELETE_START_SEMA_ENTER,
        (uint32_t)consumer.start_wait_semaphore_id,
        0u);
    if (consumer.start_wait_semaphore_id >= 0) {
        stage_result = DeleteSema(consumer.start_wait_semaphore_id) >= 0;
        consumer.start_wait_semaphore_id = -1;
    } else {
        stage_result = 1;
    }
    telemetry_emit(
        STAGE_DELETE_START_SEMA_RETURN,
        (uint32_t)stage_result,
        0u);

    telemetry_emit(
        STAGE_DELETE_SNAPSHOT_SEMA_ENTER,
        (uint32_t)consumer.snapshot_ready_semaphore_id,
        0u);
    if (consumer.snapshot_ready_semaphore_id >= 0) {
        stage_result = DeleteSema(
            consumer.snapshot_ready_semaphore_id) >= 0;
        consumer.snapshot_ready_semaphore_id = -1;
    } else {
        stage_result = 1;
    }
    telemetry_emit(
        STAGE_DELETE_SNAPSHOT_SEMA_RETURN,
        (uint32_t)stage_result,
        0u);

    telemetry_emit(
        STAGE_FLOW_RELEASE_ENTER,
        (uint32_t)flow_live,
        (uint32_t)consumer_thread_id);
    if (flow_live && consumer_thread_id < 0)
        stage_result = pstvnc_transport_rfb_flow_release(&flow);
    else
        stage_result = 1;
    telemetry_emit(
        STAGE_FLOW_RELEASE_RETURN,
        (uint32_t)stage_result,
        (uint32_t)flow.failed);

    telemetry_emit(
        STAGE_PHYSICAL_RELEASE_ENTER,
        (uint32_t)stream_live,
        (uint32_t)stream.socket_fd);
    if (stream_live)
        pstvnc_transport_physical_stream_release(&stream);
    telemetry_emit(
        STAGE_PHYSICAL_RELEASE_RETURN,
        (uint32_t)stream.socket_fd,
        (uint32_t)stream.send_semaphore_id);

    telemetry_emit(
        STAGE_NETWORK_CLOSE_ENTER,
        (uint32_t)socket_fd,
        0u);
    if (socket_fd >= 0)
        pstvnc_ps2_network_close(socket_fd);
    telemetry_emit(
        STAGE_NETWORK_CLOSE_RETURN,
        (uint32_t)socket_fd,
        0u);

    if (!success)
        return 1;

    telemetry_emit(
        STAGE_OSDSYS_ENTER,
        final_activity_sequence,
        (uint32_t)final_available);
    pstvnc_ps2_system_exit_to_menu();
    return 0;
}
