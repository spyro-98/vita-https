/* Actual lifecycle/status entry points: NetCtl and curl become observable only
 * after initialization publishes its completed runtime to other threads. */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "../src/vita_https.c"
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int entered, proceed, net_payload, curl_payload, finished, stop_readers;
int vita_https_net_init(void) {
    pthread_mutex_lock(&gate); entered = 1; pthread_cond_broadcast(&changed);
    while (!proceed) pthread_cond_wait(&changed, &gate);
    pthread_mutex_unlock(&gate); net_payload = 42; return 0;
}
CURLcode curl_global_init(long flags) { assert(flags == CURL_GLOBAL_DEFAULT); curl_payload = 84; return CURLE_OK; }
void curl_global_cleanup(void) { assert(curl_payload == 84); curl_payload = 0; }
void vita_https_net_term(void) { assert(net_payload == 42); net_payload = 0; }
int vita_https_net_is_connected(void) { assert(net_payload == 42 && curl_payload == 84); return 1; }
int vita_https_net_wifi_signal_percent(int *out) { assert(net_payload == 42 && curl_payload == 84); *out = 75; return 0; }
static void *initialize(void *unused) {
    (void)unused; assert(!vita_https_init());
    __atomic_store_n(&finished, 1, __ATOMIC_RELEASE); return NULL;
}
static void *read_status(void *unused) {
    (void)unused;
    while (!__atomic_load_n(&stop_readers, __ATOMIC_ACQUIRE)) {
        int percent = -1;
        assert(vita_https_is_connected() == 0 || vita_https_is_connected() == 1);
        int ret = vita_https_wifi_signal_percent(&percent);
        assert((ret == 0 && percent == 75) || ret == VITA_HTTPS_ERROR_NOT_INITIALIZED);
        VitaHttpsClient *client = vita_https_client_create(NULL);
        vita_https_client_destroy(client);
    }
    return NULL;
}
int main(void) {
    alarm(10);
    pthread_t init, readers[3]; assert(!pthread_create(&init, NULL, initialize, NULL));
    pthread_mutex_lock(&gate); while (!entered) pthread_cond_wait(&changed, &gate); pthread_mutex_unlock(&gate);
    assert(!vita_https_is_connected() && !vita_https_client_create(NULL));
    int percent = 0; assert(vita_https_wifi_signal_percent(&percent) == VITA_HTTPS_ERROR_NOT_INITIALIZED);
    for (unsigned i = 0; i < 3; i++) assert(!pthread_create(&readers[i], NULL, read_status, NULL));
    pthread_mutex_lock(&gate); proceed = 1; pthread_cond_broadcast(&changed); pthread_mutex_unlock(&gate);
    while (!__atomic_load_n(&finished, __ATOMIC_ACQUIRE)) usleep(100);
    assert(!pthread_join(init, NULL) && vita_https_is_connected());
    usleep(10000); __atomic_store_n(&stop_readers, 1, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < 3; i++) assert(!pthread_join(readers[i], NULL));
    assert(!vita_https_init()); vita_https_shutdown(); assert(vita_https_is_connected());
    vita_https_shutdown(); assert(!vita_https_is_connected() && !vita_https_client_create(NULL));
    puts("HTTPS startup: pending initialization hides incomplete NetCtl/curl state; concurrent status/client reads, publication, refcount and ordered shutdown passed.");
}
