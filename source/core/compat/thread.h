/* thread.h - Cross-platform threading primitives
 *
 * Provides portable mutex and thread primitives for Windows and POSIX.
 * Uses CRITICAL_SECTION on Windows and pthread_mutex on POSIX.
 *
 * Usage:
 *   psnd_mutex_t mutex;
 *   psnd_mutex_init(&mutex);
 *   psnd_mutex_lock(&mutex);
 *   // ... critical section ...
 *   psnd_mutex_unlock(&mutex);
 *   psnd_mutex_destroy(&mutex);
 */

#ifndef PSND_THREAD_H
#define PSND_THREAD_H

#ifdef _WIN32

/* ======================= Windows Threading ================================ */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdlib.h>  /* For malloc/free in thread wrapper */

/* Mutex */
typedef CRITICAL_SECTION psnd_mutex_t;

static inline int psnd_mutex_init(psnd_mutex_t *m) {
    InitializeCriticalSection(m);
    return 0;
}

static inline void psnd_mutex_destroy(psnd_mutex_t *m) {
    DeleteCriticalSection(m);
}

static inline void psnd_mutex_lock(psnd_mutex_t *m) {
    EnterCriticalSection(m);
}

static inline void psnd_mutex_unlock(psnd_mutex_t *m) {
    LeaveCriticalSection(m);
}

static inline int psnd_mutex_trylock(psnd_mutex_t *m) {
    return TryEnterCriticalSection(m) ? 0 : -1;
}

/* Thread */
typedef HANDLE psnd_thread_t;

/* Wrapper to bridge POSIX thread signature to Windows calling convention.
 * POSIX: void* (*)(void*)  - cdecl, returns pointer
 * Windows: unsigned int (__stdcall *)(void*) - stdcall, returns uint
 * Direct casting between these causes stack corruption on x86. */
typedef struct {
    void *(*func)(void *);
    void *arg;
} psnd_thread_wrapper_t;

static unsigned int __stdcall psnd_thread_wrapper_func(void *wrapper_arg) {
    psnd_thread_wrapper_t *wrapper = (psnd_thread_wrapper_t *)wrapper_arg;
    void *(*func)(void *) = wrapper->func;
    void *arg = wrapper->arg;
    free(wrapper);  /* Free before calling so we don't leak on thread exit */
    func(arg);
    return 0;
}

static inline int psnd_thread_create(psnd_thread_t *thread, void *(*func)(void *), void *arg) {
    psnd_thread_wrapper_t *wrapper = (psnd_thread_wrapper_t *)malloc(sizeof(*wrapper));
    if (!wrapper) return -1;
    wrapper->func = func;
    wrapper->arg = arg;
    *thread = (HANDLE)_beginthreadex(NULL, 0, psnd_thread_wrapper_func, wrapper, 0, NULL);
    if (*thread == NULL) {
        free(wrapper);
        return -1;
    }
    return 0;
}

static inline int psnd_thread_join(psnd_thread_t thread, void **retval) {
    (void)retval;  /* Windows doesn't easily support return values */
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

static inline void psnd_thread_yield(void) {
    SwitchToThread();
}

/* Condition variable (Windows Vista+) */
typedef CONDITION_VARIABLE psnd_cond_t;

static inline int psnd_cond_init(psnd_cond_t *c) {
    InitializeConditionVariable(c);
    return 0;
}

static inline void psnd_cond_destroy(psnd_cond_t *c) {
    (void)c;  /* No cleanup needed */
}

static inline void psnd_cond_signal(psnd_cond_t *c) {
    WakeConditionVariable(c);
}

static inline void psnd_cond_broadcast(psnd_cond_t *c) {
    WakeAllConditionVariable(c);
}

static inline int psnd_cond_wait(psnd_cond_t *c, psnd_mutex_t *m) {
    return SleepConditionVariableCS(c, m, INFINITE) ? 0 : -1;
}

#else

/* ======================= POSIX Threading ================================== */

#include <pthread.h>

/* Mutex */
typedef pthread_mutex_t psnd_mutex_t;

static inline int psnd_mutex_init(psnd_mutex_t *m) {
    return pthread_mutex_init(m, NULL);
}

static inline void psnd_mutex_destroy(psnd_mutex_t *m) {
    pthread_mutex_destroy(m);
}

static inline void psnd_mutex_lock(psnd_mutex_t *m) {
    pthread_mutex_lock(m);
}

static inline void psnd_mutex_unlock(psnd_mutex_t *m) {
    pthread_mutex_unlock(m);
}

static inline int psnd_mutex_trylock(psnd_mutex_t *m) {
    return pthread_mutex_trylock(m);
}

/* Thread */
typedef pthread_t psnd_thread_t;

static inline int psnd_thread_create(psnd_thread_t *thread, void *(*func)(void *), void *arg) {
    return pthread_create(thread, NULL, func, arg);
}

static inline int psnd_thread_join(psnd_thread_t thread, void **retval) {
    return pthread_join(thread, retval);
}

static inline void psnd_thread_yield(void) {
    sched_yield();
}

/* Condition variable */
typedef pthread_cond_t psnd_cond_t;

static inline int psnd_cond_init(psnd_cond_t *c) {
    return pthread_cond_init(c, NULL);
}

static inline void psnd_cond_destroy(psnd_cond_t *c) {
    pthread_cond_destroy(c);
}

static inline void psnd_cond_signal(psnd_cond_t *c) {
    pthread_cond_signal(c);
}

static inline void psnd_cond_broadcast(psnd_cond_t *c) {
    pthread_cond_broadcast(c);
}

static inline int psnd_cond_wait(psnd_cond_t *c, psnd_mutex_t *m) {
    return pthread_cond_wait(c, m);
}

#endif /* _WIN32 */

#endif /* PSND_THREAD_H */
