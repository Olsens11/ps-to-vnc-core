/*
 * Host tests for Transport's backend-independent logical RFB byte storage.
 * These tests exercise capacity rejection, circular wrap, incremental parser
 * consumption, exact reads, producer activity, and terminal residual discard.
 */

#include <stdio.h>
#include <string.h>

#include "transport/rfb_channel.h"

static int failures = 0;

#define CHECK(expr)                                                     \
    do {                                                                \
        if (!(expr)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n",                         \
                    __FILE__, __LINE__, #expr);                         \
            failures++;                                                 \
        }                                                               \
    } while (0)

static void test_initialize_and_capacity_rejection(void)
{
    unsigned char storage[8];
    static const unsigned char first[] = { 1, 2, 3, 4, 5, 6 };
    static const unsigned char overflow[] = { 7, 8, 9 };
    pstvnc_transport_rfb_channel_t channel;

    CHECK(pstvnc_transport_rfb_channel_initialize(
        &channel, storage, sizeof(storage)) == 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == 0u);
    CHECK(pstvnc_transport_rfb_channel_activity_generation(&channel) == 0u);

    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, first, sizeof(first)) == 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == sizeof(first));
    CHECK(pstvnc_transport_rfb_channel_activity_generation(&channel) == 1u);

    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, overflow, sizeof(overflow)) != 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == sizeof(first));
    CHECK(pstvnc_transport_rfb_channel_activity_generation(&channel) == 1u);
}

static void test_incremental_wraparound_consumption(void)
{
    unsigned char storage[8];
    unsigned char output[8];
    static const unsigned char first[] = { 1, 2, 3, 4, 5, 6 };
    static const unsigned char second[] = { 7, 8, 9, 10, 11 };
    static const unsigned char expected[] = { 5, 6, 7, 8, 9, 10, 11 };
    pstvnc_transport_rfb_channel_t channel;
    size_t taken;

    CHECK(pstvnc_transport_rfb_channel_initialize(
        &channel, storage, sizeof(storage)) == 0);
    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, first, sizeof(first)) == 0);

    memset(output, 0, sizeof(output));
    taken = pstvnc_transport_rfb_channel_read_available(
        &channel, output, 4u);
    CHECK(taken == 4u);
    CHECK(memcmp(output, first, 4u) == 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == 2u);

    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, second, sizeof(second)) == 0);
    CHECK(pstvnc_transport_rfb_channel_activity_generation(&channel) == 2u);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == 7u);

    memset(output, 0, sizeof(output));
    taken = pstvnc_transport_rfb_channel_read_available(
        &channel, output, sizeof(output));
    CHECK(taken == sizeof(expected));
    CHECK(memcmp(output, expected, sizeof(expected)) == 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == 0u);
}

static void test_exact_read_is_atomic_on_short_input(void)
{
    unsigned char storage[8];
    unsigned char output[4] = { 0xaa, 0xaa, 0xaa, 0xaa };
    static const unsigned char input[] = { 1, 2, 3 };
    pstvnc_transport_rfb_channel_t channel;

    CHECK(pstvnc_transport_rfb_channel_initialize(
        &channel, storage, sizeof(storage)) == 0);
    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, input, sizeof(input)) == 0);
    CHECK(pstvnc_transport_rfb_channel_read_exact(
        &channel, output, sizeof(output)) != 0);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == sizeof(input));
    CHECK(output[0] == 0xaa);
}

static void test_terminal_residual_discard_is_distinct(void)
{
    unsigned char storage[8];
    static const unsigned char input[] = { 1, 2, 3, 4, 5 };
    pstvnc_transport_rfb_channel_t channel;
    size_t discarded = 99u;
    uint32_t generation;

    CHECK(pstvnc_transport_rfb_channel_initialize(
        &channel, storage, sizeof(storage)) == 0);
    CHECK(pstvnc_transport_rfb_channel_commit(
        &channel, input, sizeof(input)) == 0);
    generation = pstvnc_transport_rfb_channel_activity_generation(&channel);

    CHECK(pstvnc_transport_rfb_channel_discard_residual(
        &channel, sizeof(input) - 1u, &discarded) != 0);
    CHECK(discarded == 0u);
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == sizeof(input));

    CHECK(pstvnc_transport_rfb_channel_discard_residual(
        &channel, sizeof(input), &discarded) == 0);
    CHECK(discarded == sizeof(input));
    CHECK(pstvnc_transport_rfb_channel_available(&channel) == 0u);
    CHECK(pstvnc_transport_rfb_channel_activity_generation(&channel) == generation);
}

int main(void)
{
    test_initialize_and_capacity_rejection();
    test_incremental_wraparound_consumption();
    test_exact_read_is_atomic_on_short_input();
    test_terminal_residual_discard_is_distinct();

    if (failures != 0) {
        fprintf(stderr, "transport_rfb_channel_test: %d failure(s)\n", failures);
        return 1;
    }

    printf("transport_rfb_channel_test: PASS\n");
    return 0;
}
