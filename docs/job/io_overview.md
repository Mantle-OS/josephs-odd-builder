# Job IO

The io stack of job

This module builds as `jobio` and links against `job_core` and `job_threads`.
job:
- `job::core` defines the job::core::IODevice because it is shared with `job::uart`
- `job::threads` defines the job::threads::JobIoAsyncThread some io backends use this


## IO factory (`IOFactory`)
Dispatcher that creates an IODevice from a string form:

- `pty:` creates the PTY backend
- `file:stdin|stdout|stderr` maps to standard streams
- `file:/path/to/file` maps to a file backend


## File I/O (`FileIO`)
A wrapper around:
- files (binary read/write through C++ streams)
- standard streams (stdin/stdout/stderr)

The “file” side is synchronous I/O with a mutex around the stream state. 
The stdio side routes into `std::cin` / `std::cout` / `std::cerr` and keeps 
them open (no ownership of global streams).

## Pseudo-terminal I/O (`PtyIO`)
A PTY-backed device:
- opens a PTY master/slave pair
- configures termios defaults + window size
- optionally forks and execs a login shell attached to the slave side
- reads asynchronously from the master side using the epoll-based async thread from `job::threads::JobIoAsyncThread`)
- dispatches read chunks via a read callback, and process exit via an exit callback

This backend is the bridge between:
- “a file descriptor that emits data sometimes”
and
- “a callback-driven stream”

## Shared memory
POSIX shared memory segment (`shm_open` + `mmap`) with a header followed by a byte ring buffer.

The layout is:
- a fixed-size header
- the ring buffer region

Signaling is done with a named semaphore derived from the shared memory key (key + `_sig`), 
used as a data arrived signal for the reader. 

The reader blocks on that signal when empty (unless nonblocking mode is enabled).

This is shaped around a simple producer/consumer stream:
- writer advances the write position and posts the semaphore
- reader waits for data and advances the read position
- wrap-around is handled at the memcpy boundary

The header is deliberately small and self-checking (magic/version/size validation) so readers can reject invalid mappings.

## Mock I/O (`MockIO`)
A memory-backed IODevice for tests and small harnesses:
- an internal read buffer (deque) inbound bytes
- an internal write buffer (string) collects outbound bytes
