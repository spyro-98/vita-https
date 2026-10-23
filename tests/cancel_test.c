#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#ifndef VMD_HTTPS_SOURCE
#define VMD_HTTPS_SOURCE "../src/vita_https.c"
#endif
#include VMD_HTTPS_SOURCE

static volatile int stop;
static int ready, done;
static void *observe(void *unused) {
    (void)unused;
    ProgressBridge bridge = {.cancel = &stop};
    __atomic_store_n(&ready, 1, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < 200000; ++i) {
        int a = progress_cancel((void *)&stop, 0, 0, 0, 0);
        int b = progress_bridge(&bridge, 0, 0, 0, 0);
        assert((a == 0 || a == 1) && (b == 0 || b == 1));
    }
    __atomic_store_n(&done, 1, __ATOMIC_RELEASE);
    return NULL;
}
int main(void) {
    ProgressBridge bridge = {.cancel = &stop};
    assert(!progress_cancel(NULL, 0, 0, 0, 0));
    assert(progress_bridge(NULL, 0, 0, 0, 0));
    assert(!progress_bridge(&bridge, 0, 0, 0, 0));
    __atomic_store_n(&stop, 1, __ATOMIC_RELEASE);
    assert(progress_cancel((void *)&stop, 0, 0, 0, 0));
    assert(progress_bridge(&bridge, 0, 0, 0, 0));
    pthread_t thread;
    assert(!pthread_create(&thread, NULL, observe, NULL));
    while (!__atomic_load_n(&ready, __ATOMIC_ACQUIRE)) usleep(100);
    unsigned value = 0;
    while (!__atomic_load_n(&done, __ATOMIC_ACQUIRE))
        __atomic_store_n(&stop, (int)(value++ & 1), __ATOMIC_RELEASE);
    assert(!pthread_join(thread, NULL));
    puts("HTTPS cancellation: actual transport callbacks, 200000 concurrent reads passed");
}
