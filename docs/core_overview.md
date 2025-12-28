# Job Core

The "core" stack of job.

`job::core` is the shared base layer used by everything else (threads / io / uart / net / serializer / etc).

## JobLogger
Small logger with levels and a timestamped line format.

Used by macros (`JOB_LOG_*`) so the call sites stay short.

## JobTimer
Lightweight timer object used by loops/schedulers.

Holds:
- next fire time
- interval
- repeat/active flags
- callback

## IOPermissions
POSIX mode-bit wrapper + a few named presets.
- named permission presets (file/dir/private/etc)
- octal + symbolic formatting

## IODevice
Abstract base for IO, Uart backends.
- open/close
- read/write
- fd/nonblocking
- optional read callback
- permission tracking

This is shared with `job::uart` and `job::io`.

## Endian helpers
Small helpers for little-endian conversion.

## CRC32
CRC32 checksums slash integrity.

## real_t
### legacy will be removed.
Single “float vs double” getting phased out.

## SplitMix64
PRNG used for sampling / seeding. Not Crypto

## Assert (`JOB_ASSERT`)
not in use at the moment
