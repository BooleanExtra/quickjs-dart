/*
 * Minimal libwinpthread-1.dll stub for Windows x64.
 *
 * quickjs-windows-x64.dll is compiled with MinGW and imports from
 * libwinpthread-1.dll. The Windows system libwinpthread-1.dll (if present)
 * may be an old version that lacks clock_gettime64 and pthread_cond_timedwait64.
 * libgcc_s_seh-1.dll also imports additional functions from libwinpthread-1.dll.
 *
 * This stub provides all required exports with correct implementations for
 * single-threaded use (one QuickJS context per thread, no actual concurrency).
 *
 * Compile with MinGW:
 *   gcc -shared -o libwinpthread-1.dll libwinpthread_stub.c \
 *       -nostdlib -lkernel32 -Wl,--kill-at
 */

#include <windows.h>
#include <stdint.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    return TRUE;
}

/* clock_gettime64 — POSIX realtime clock via GetSystemTimePreciseAsFileTime */
typedef int clockid_t;
struct _timespec64 {
    int64_t tv_sec;
    int64_t tv_nsec;
};

__declspec(dllexport) int clock_gettime64(clockid_t clk_id, struct _timespec64 *tp) {
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    t -= 116444736000000000ULL; /* 100ns intervals from 1601-01-01 to 1970-01-01 */
    tp->tv_sec  = (int64_t)(t / 10000000ULL);
    tp->tv_nsec = (int64_t)((t % 10000000ULL) * 100LL);
    return 0;
}

/* pthread types — sized to match MinGW's ABI */
typedef struct { void *p[10]; long i; } pthread_mutex_t;
typedef struct { void *p[10]; } pthread_cond_t;
typedef struct { void *p[4]; } pthread_condattr_t;
typedef struct { void *p[4]; } pthread_mutexattr_t;
typedef struct { int64_t tv_sec; int64_t tv_nsec; } _timespec64_abs;
typedef int pthread_key_t;
typedef volatile int pthread_once_t;

/* Thread-local storage — simple array for single-threaded use */
#define MAX_TLS_KEYS 64
static void *_tls_values[MAX_TLS_KEYS];
static void (*_tls_destructors[MAX_TLS_KEYS])(void *);
static int _tls_count = 0;

__declspec(dllexport) int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    if (_tls_count >= MAX_TLS_KEYS) return 12; /* ENOMEM */
    *key = _tls_count;
    _tls_values[_tls_count] = NULL;
    _tls_destructors[_tls_count] = destructor;
    _tls_count++;
    return 0;
}

__declspec(dllexport) void *pthread_getspecific(pthread_key_t key) {
    if (key < 0 || key >= _tls_count) return NULL;
    return _tls_values[key];
}

__declspec(dllexport) int pthread_setspecific(pthread_key_t key, const void *value) {
    if (key < 0 || key >= _tls_count) return 22; /* EINVAL */
    _tls_values[key] = (void *)value;
    return 0;
}

__declspec(dllexport) int pthread_once(pthread_once_t *control, void (*init_fn)(void)) {
    if (*control == 0) {
        *control = 1;
        init_fn();
    }
    return 0;
}

/* Mutex stubs — no-op for single-threaded QuickJS use */
__declspec(dllexport) int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) { (void)mutex; (void)attr; return 0; }
__declspec(dllexport) int pthread_mutex_lock(pthread_mutex_t *mutex) { (void)mutex; return 0; }
__declspec(dllexport) int pthread_mutex_unlock(pthread_mutex_t *mutex) { (void)mutex; return 0; }

/* Condition variable stubs — no-op for single-threaded QuickJS use */
__declspec(dllexport) int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) { (void)cond; (void)attr; return 0; }
__declspec(dllexport) int pthread_cond_destroy(pthread_cond_t *cond) { (void)cond; return 0; }
__declspec(dllexport) int pthread_cond_signal(pthread_cond_t *cond) { (void)cond; return 0; }
__declspec(dllexport) int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) { (void)cond; (void)mutex; return 0; }
__declspec(dllexport) int pthread_cond_timedwait64(pthread_cond_t *cond, pthread_mutex_t *mutex, const _timespec64_abs *abstime) { (void)cond; (void)mutex; (void)abstime; return 0; }
