# vita-https

Small, secure-by-default HTTPS transport package for PlayStation Vita. It owns
the sceNet/sceNetCtl lifecycle, a pinned libcurl/Mbed TLS stack, Mozilla CA
bundle, authenticated requests and cached seekable Range streams.
Connectivity and Wi-Fi signal queries are guarded by the same lifecycle, so
UI code never calls an unloaded sceNetCtl stub.

It is the extracted transport used by VitaMediaDeck. The protocol is not a custom
TLS implementation: TLS and certificate validation are provided by Mbed TLS;
this package owns the Vita lifecycle, policy and stream abstraction around it.

## Integrate

Build the isolated TLS dependency first. The system VitaSDK libcurl/OpenSSL
archive is deliberately rejected because the installed OpenSSL 1.0.2 port is
end-of-life and has licensing compatibility problems with GPLv3 binaries.

```sh
export VITASDK=/path/to/vitasdk
./tools/build-curl-mbedtls.sh
```

```cmake
add_subdirectory(external/vita-https)
target_link_libraries(my_app PRIVATE VitaHttps::VitaHttps)
```

```c
#include <vita_https.h>

vita_https_init();
VitaHttpsClient *client = vita_https_client_create(NULL);
VitaHttpsRequest request = { .method = "GET", .url = "https://example.com" };
VitaHttpsResponse response;
int result = vita_https_perform(client, &request, &response);
vita_https_client_destroy(client);
vita_https_shutdown();
```

`examples/get.c` includes response buffering. `vita_https_open_range_stream`
returns a seekable 512 KiB-cached cursor after one real HTTP `206 Partial
Content` request validates the server, obtains the total size, and fills the
first cache window. The cursor retains and reuses its libcurl easy handle and
connection cache for later range fills; failed fills invalidate the old window
before any partially received bytes can be observed. Its five cursor fields map
directly onto the decoder packages' stream handle.

## VitaMediaDeck integration

VitaMediaDeck uses the generic request API for authenticated WebDAV operations
and the Range-stream API for media access. Independent stream handles can feed
cover/thumbnail probing, H.264 video, the selected AAC audio track, and an
explicitly requested embedded text-subtitle track without sharing mutable seek
positions. Selectable-track metadata comes from the already-open video demux,
so startup does not create another discovery cursor. The app keeps passwords
session-only by default; thumbnail
caches contain decoded RGB565 pixels and never serialize credentials.

The package remains protocol-neutral. WebDAV XML parsing, saved-source policy,
player resume history, and UI belong to VitaMediaDeck rather than this HTTPS
transport layer.

## Security contract

- `https://` is the default and HTTPS redirects can never downgrade to HTTP;
- plain `http://` is accepted only when a caller explicitly sets `allow_http`;
  credentials and response data are unencrypted in that mode;
- TLS 1.2 is the minimum negotiated protocol version;
- peer and hostname verification are enabled by default;
- a bundled Mozilla CA store is supplied explicitly;
- credentials are never placed in URLs;
- redirects, timeouts, low-speed aborts and cooperative cancellation are
  bounded by the package;
- private or self-signed peers require an exact `sha256//` SPKI pin on every
  request; there is no mode that disables CA verification without a pin.

`vita_https_probe_public_key` performs an unauthenticated, bodyless handshake
solely to obtain the leaf public-key pin for an explicit trust-on-first-use UI.
Do not use the probe result for application data until the user has compared
and confirmed it. A client configured with that confirmed pin may set
`allow_untrusted_ca_with_pin`; the exact public key then replaces public-CA and
hostname trust for that private endpoint, and any later key change fails closed.

The client must outlive every Range stream created from it. Call init/shutdown
symmetrically; the global lifecycle is reference-counted.

## Install and consume

```sh
cmake -S . -B build/package
cmake --build build/package
cmake --install build/package --prefix "$PWD/build/stage"
```

Installed consumers may use `find_package(VitaHttps CONFIG REQUIRED)` and link
`VitaHttps::VitaHttps`. The installed package carries the pinned libcurl
archive, its license texts and corresponding upstream source; Mbed TLS,
zlib/zstd and Vita system stubs are supplied by VitaSDK.

## Related repositories

| Repository | Relationship |
| --- | --- |
| [`VitaMediaDeck`](https://github.com/spyro-98/VitaMediaDeck) | Main local and authenticated-network media application |
| [`vita-hw-decoder`](https://github.com/spyro-98/vita-hw-decoder) | Hardware H.264/AAC consumer of seekable stream factories |
| [`vita-sw-decoder`](https://github.com/spyro-98/vita-sw-decoder) | CPU H.264 fallback using the same stream contract |
| [`VitaMediaDeck-Transcoder`](https://github.com/spyro-98/VitaMediaDeck-Transcoder) | Optional desktop producer of Vita-oriented media files |

Licensed GPL-3.0-only. See `THIRD_PARTY_NOTICES.md`.
