# HTTPS cancellation regression (macOS)

```sh
VITASDK=/path/to/vitasdk bash tests/run-host-tests.sh
```

The harness includes the real transport implementation and calls both curl
cancellation callbacks while a second thread writes the cancellation flag.
It runs under ASan/UBSan and ThreadSanitizer. Mbed TLS headers are taken from
the SDK; unused transport functions are removed by the host linker. No Vita
binaries, TLS library, network connection or console are used by this test.

The previous non-atomic reads reproduce a ThreadSanitizer data race. This
regression preserves the existing public ABI and covers null flags, unset/set
flags and 200,000 concurrent callback reads.
