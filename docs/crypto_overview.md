# Job Crypto

The crypto of job.

jobcrypto links against libsodium and `job_core`.

job:
- `job::crypto` wraps libsodium init + secure memory + random
- `job::core` provides logging + `real_t`

## JobCryptoInit

One-time libsodium init flag.
- `sodium_init()` happens once
- used by `JobRandom` and `JobSecureMem`

## JobRandom
Random utilities split into two lanes:
- secure bytes (libsodium `randombytes_*`)
- per-thread PRNG (`std::mt19937_64`) seeded from secure bytes (or from a global seed)

includes basic distributions (uniform / normal / bernoulli) using `job::core::real_t`.

Global seed mode exists for repeatable runs (thread-local engines still get derived seeds).

## JobSecureMem
Secure buffer wrapper using libsodium:
- allocation uses `sodium_malloc`
- memory is locked + zeroed on allocate
- compares use constant-time compare
- supports clearing and free (zero then free)

String conversion:
- base64 helpers exist (`toBase64`, `fromBase64`)
- `toString()` is gated behind `JOB_SECUREMEM_ALLOW_STRING` (disabled in secure builds)

Used as the “don’t leave plaintext in heap” type (ex: `job::net::JobUrl` password storage).
