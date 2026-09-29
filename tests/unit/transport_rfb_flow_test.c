/*
 * Host fixture for the extracted synchronized RFB activity rendezvous.
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "transport_host_stubs/kernel.h"
#include "transport/rfb_flow.h"

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); failures++; } } while (0)

unsigned char _gp;

typedef struct test_sema {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int count;
    int max_count;
    int live;
} test_sema_t;

static test_sema_t semas[8];
static pthread_mutex_t watch_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t watch_cond = PTHREAD_COND_INITIALIZER;
static int watched_sema_id = -1;
static int watched_waiter_entered;

int CreateSema(ee_sema_t *s)
{
    int id;
    for (id = 1; id < 8; id++) {
        if (!semas[id].live) {
            pthread_mutex_init(&semas[id].lock, NULL);
            pthread_cond_init(&semas[id].cond, NULL);
            semas[id].count = s->init_count;
            semas[id].max_count = s->max_count;
            semas[id].live = 1;
            return id;
        }
    }
    return -1;
}

int DeleteSema(int id)
{
    if (id <= 0 || id >= 8 || !semas[id].live) return -1;
    semas[id].live = 0;
    return 0;
}

int WaitSema(int id)
{
    test_sema_t *s;
    if (id <= 0 || id >= 8 || !semas[id].live) return -1;
    s = &semas[id];
    pthread_mutex_lock(&s->lock);
    while (s->count == 0) {
        if (id == watched_sema_id) {
            pthread_mutex_lock(&watch_lock);
            watched_waiter_entered = 1;
            pthread_cond_signal(&watch_cond);
            pthread_mutex_unlock(&watch_lock);
        }
        pthread_cond_wait(&s->cond, &s->lock);
    }
    s->count--;
    pthread_mutex_unlock(&s->lock);
    return 0;
}

int SignalSema(int id)
{
    test_sema_t *s;
    if (id <= 0 || id >= 8 || !semas[id].live) return -1;
    s = &semas[id];
    pthread_mutex_lock(&s->lock);
    if (s->count >= s->max_count) {
        pthread_mutex_unlock(&s->lock);
        return -1;
    }
    s->count++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->lock);
    return 0;
}

int PollSema(int id) { (void)id; return -1; }
int CreateThread(ee_thread_t *t) { (void)t; return -1; }
int StartThread(int id, void *a) { (void)id; (void)a; return -1; }
int DeleteThread(int id) { (void)id; return -1; }
int ReferThreadStatus(int id, ee_thread_status_t *s) { (void)id; (void)s; return -1; }
int TerminateThread(int id) { (void)id; return -1; }
void ExitThread(void) {}

typedef struct waiter_ctx {
    pstvnc_transport_rfb_flow_t *flow;
    uint32_t sequence;
    int result;
} waiter_ctx_t;

static void *waiter(void *opaque)
{
    waiter_ctx_t *ctx = (waiter_ctx_t *)opaque;
    ctx->result = pstvnc_transport_rfb_flow_wait_activity(ctx->flow, &ctx->sequence);
    return NULL;
}

int main(void)
{
    unsigned char storage[32];
    unsigned char output[4];
    static const unsigned char payload[] = {1,2,3,4};
    pstvnc_transport_rfb_flow_t flow;
    waiter_ctx_t ctx;
    pthread_t thread;

    memset(&flow, 0, sizeof(flow));
    CHECK(pstvnc_transport_rfb_flow_initialize(&flow, storage, sizeof(storage)) == 1);
    CHECK(pstvnc_transport_rfb_flow_activity_snapshot(&flow, &ctx.sequence) == 1);

    ctx.flow = &flow;
    ctx.result = 0;
    watched_sema_id = flow.activity_semaphore_id;
    watched_waiter_entered = 0;
    CHECK(pthread_create(&thread, NULL, waiter, &ctx) == 0);

    pthread_mutex_lock(&watch_lock);
    while (!watched_waiter_entered)
        pthread_cond_wait(&watch_cond, &watch_lock);
    pthread_mutex_unlock(&watch_lock);

    CHECK(pstvnc_transport_rfb_flow_commit_data(&flow, payload, sizeof(payload)) == 1);
    CHECK(pthread_join(thread, NULL) == 0);
    CHECK(ctx.result == 1);
    CHECK(ctx.sequence == 1u);
    CHECK(pstvnc_transport_rfb_flow_read_exact(&flow, output, sizeof(output)) == 1);
    CHECK(memcmp(output, payload, sizeof(payload)) == 0);

    CHECK(pstvnc_transport_rfb_flow_activity_snapshot(&flow, &ctx.sequence) == 1);
    CHECK(pstvnc_transport_rfb_flow_commit_data(&flow, payload, sizeof(payload)) == 1);
    CHECK(pstvnc_transport_rfb_flow_wait_activity(&flow, &ctx.sequence) == 1);
    CHECK(ctx.sequence == 2u);

    CHECK(pstvnc_transport_rfb_flow_release(&flow) == 1);

    if (failures) {
        fprintf(stderr, "transport_rfb_flow_test: %d failure(s)\n", failures);
        return 1;
    }
    puts("transport_rfb_flow_test: PASS");
    return 0;
}
