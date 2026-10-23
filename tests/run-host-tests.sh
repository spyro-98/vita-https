#!/usr/bin/env bash
set -euo pipefail
vita_https_root="$(cd "$(dirname "$0")/.." && pwd)"
: "${VITASDK:?Point VITASDK at SDK headers for Mbed TLS; no Vita binaries are linked}"
vita_https_output="$(mktemp -d "${TMPDIR:-/tmp}/vita-https-cancel.XXXXXX")"
mkdir -p "$vita_https_output/include"
for vita_https_headers in mbedtls psa; do
    ln -s "$VITASDK/arm-vita-eabi/include/$vita_https_headers" "$vita_https_output/include/$vita_https_headers"
done
cd "$vita_https_root"
for vita_https_sanitizer in address,undefined thread; do
    clang -O1 -g -Wall -Wextra -Wno-deprecated-declarations \
        "-fsanitize=$vita_https_sanitizer" -ffunction-sections -fdata-sections \
        -Iinclude "-I$vita_https_output/include" tests/cancel_test.c \
        -Wl,-dead_strip -lpthread -o "$vita_https_output/cancel-test"
    "$vita_https_output/cancel-test"
done
printf 'HTTPS cancellation host artifacts: %s\n' "$vita_https_output"
