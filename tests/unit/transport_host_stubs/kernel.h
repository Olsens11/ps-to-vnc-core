/*
 * File synopsis:
 * Minimal host-only PS2 kernel declarations used by the direct A001 Transport
 * behavior fixtures. Implementations live in each fixture so failure and
 * synchronization behavior stays explicit and deterministic.
 *
 * PS2SDK's ee_thread_t stores its entry point in a void * field and product code
 * therefore uses the SDK-required function-pointer/object-pointer conversion.
 * ISO C diagnoses that representation under -pedantic even though it is the
 * target ABI contract. Suppress only that host-language diagnostic in this
 * test-only shim; -Wall/-Wextra and -Werror remain active for all other classes.
 */

#ifndef PSTVNC_TEST_TRANSPORT_HOST_KERNEL_H
#define PSTVNC_TEST_TRANSPORT_HOST_KERNEL_H

#include <pthread.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

typedef struct ee_sema {
    int init_count;
    int max_count;
    int option;
} ee_sema_t;

typedef struct ee_thread {
    void *func;
    void *stack;
    int stack_size;
    void *gp_reg;
    int initial_priority;
    int attr;
    int option;
} ee_thread_t;

typedef struct ee_thread_status {
    int status;
} ee_thread_status_t;

#define THS_DORMANT 0
#define THS_RUNNING 1

extern unsigned char _gp;

int CreateSema(ee_sema_t *semaphore);
int DeleteSema(int semaphore_id);
int WaitSema(int semaphore_id);
int PollSema(int semaphore_id);
int SignalSema(int semaphore_id);

/*
 * Host model of the EE's short interrupt-disabled critical section. Product
 * runtime never blocks while this lock is held; it only protects admission/
 * count publication that must be atomic with receiver terminality.
 */
static pthread_mutex_t pstvnc_test_interrupt_mutex =
    PTHREAD_MUTEX_INITIALIZER;

static inline int DIntr(void)
{
    (void)pthread_mutex_lock(&pstvnc_test_interrupt_mutex);
    return 1;
}

static inline int EIntr(void)
{
    (void)pthread_mutex_unlock(&pstvnc_test_interrupt_mutex);
    return 0;
}

int CreateThread(ee_thread_t *thread);
int StartThread(int thread_id, void *argument);
int DeleteThread(int thread_id);
int ReferThreadStatus(int thread_id, ee_thread_status_t *status);
int TerminateThread(int thread_id);
void ExitThread(void);

#endif
