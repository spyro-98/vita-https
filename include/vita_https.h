#ifndef VITA_HTTPS_H
#define VITA_HTTPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VitaHttpsClient VitaHttpsClient;

enum {
	VITA_HTTPS_ERROR_INVALID_ARGUMENT = -1,
	VITA_HTTPS_ERROR_NOT_INITIALIZED = -2,
	VITA_HTTPS_ERROR_OUT_OF_MEMORY = -3,
	VITA_HTTPS_ERROR_HTTP_STATUS = -4,
	VITA_HTTPS_ERROR_RANGE_UNSUPPORTED = -5,
	VITA_HTTPS_ERROR_CURL_BASE = -1000
};

typedef struct VitaHttpsClientConfig {
	const char *user_agent;
	const char *username;
	const char *password;
	long connect_timeout_ms;
	long request_timeout_ms;
	long low_speed_bytes_per_second;
	long low_speed_seconds;
} VitaHttpsClientConfig;

typedef size_t (*VitaHttpsWriteCallback)(const void *data, size_t size,
	                                     void *opaque);

typedef struct VitaHttpsRequest {
	const char *method;
	const char *url;
	const char *const *headers;
	const void *body;
	size_t body_size;
	VitaHttpsWriteCallback write;
	void *write_opaque;
	volatile int *cancel_flag;
} VitaHttpsRequest;

typedef struct VitaHttpsResponse {
	long status_code;
	int64_t content_length;
	char effective_url[2048];
} VitaHttpsResponse;

/* Same cursor shape used by vita-hw-decoder and vita-sw-decoder. An adapter
 * only needs to copy these five fields into VitaDecoderStreamHandle. */
typedef struct VitaHttpsStream {
	void *opaque;
	int (*read)(void *opaque, void *buffer, size_t size);
	int64_t (*seek)(void *opaque, int64_t offset, int whence);
	void (*close)(void *opaque);
	int64_t size;
} VitaHttpsStream;

/* Owns sceNet/sceNetCtl and libcurl global state. Calls are reference counted. */
int vita_https_init(void);
void vita_https_shutdown(void);
int vita_https_is_connected(void);
int vita_https_wifi_signal_percent(int *percent);

VitaHttpsClient *vita_https_client_create(const VitaHttpsClientConfig *config);
void vita_https_client_destroy(VitaHttpsClient *client);

/* Only https:// URLs are accepted. Certificate and host verification cannot
 * be disabled through this API and redirects are restricted to HTTPS. */
int vita_https_perform(VitaHttpsClient *client,
	                   const VitaHttpsRequest *request,
	                   VitaHttpsResponse *response);

/* URL component helpers use libcurl's escaping rules. Returned strings must
 * be released with vita_https_free(). */
char *vita_https_escape(const char *text, size_t length);
char *vita_https_unescape(const char *text, size_t *length);
void vita_https_free(void *pointer);

/* Proves Range support with a real 206 response, then returns a seekable,
 * cached cursor suitable for media demuxers. The client must outlive stream. */
int vita_https_open_range_stream(VitaHttpsClient *client, const char *url,
	                             volatile int *cancel_flag,
	                             VitaHttpsStream *out);

const char *vita_https_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif
