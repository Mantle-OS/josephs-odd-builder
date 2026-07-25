# Job Net

## NOTE: not tested on windows machine I dont have one

Cross-platform (Linux + Windows) networking layer for job.

job_net links against `job_core`, `job_threads`, and `job_crypto` (the last only for TLS key/certificate handling).

- `job::net` provides URL and address parsing, an async epoll/WSAPoll-backed socket layer, DNS resolution that runs off the event loop thread, TLS context management (OpenSSL on Linux, Schannel on Windows), and client/server convenience wrappers over TCP, UDP, and Unix domain sockets
- every socket type implements one abstract interface (`ISocketIO`) and shares one connect/bind contract, so `clients/`/`servers/` code and anything built on top never has to know which transport it's actually driving
- platform divergence is isolated behind a small, named set of conflict boundaries (fd-vs-`SOCKET` handle mechanics, DNS/TLS backend, proxy interception) rather than scattered `#ifdef`s. Public headers stay free of raw platform types

## Foundational

### JobUrl
URL parser and container.

- scheme (`tcp`/`udp`/`unix`/`http`/`https`/etc.), host, port, path, query, fragment
- username/password storage, with the password held in `job::crypto::JobSecureMem` rather than a plain `std::string`. `password()` only ever returns a masked placeholder or a base64 ciphertext-ish form, never plaintext, and parsing pulls the password directly off the regex match's raw character range rather than materializing an unprotected intermediate string first
- serves as the connection description consumed by `JobResolver` and by `ISocketIO::connectToHost(JobUrl)`/`bind(JobUrl)`

### JobIpAddr
Address container for IPv4, IPv6, and Unix domain socket paths, wrapping a `sockaddr_storage` plus length, family, and port.

- `setAddress()`/`fromSockAddr()` populate it from a string or a raw `sockaddr*`; classification helpers (`isLoopback()`, `isGlobal()`, `isMulticast()`, `isBroadcast()`, `isNull()`) all operate on the parsed bytes, not the original string
- `isUnixPath()` and `isIPv4()`/`isIPv6()` are pure string/byte classification with no OS calls, kept portable and identical on both platforms
- the one platform-specific method, `isUnixPermitted()`, currently only verifies socket-ness and access on Linux (`stat`+`S_ISSOCK`+`access`); the Windows implementation checks existence only, a known, tracked gap, not yet closed

### JobHttpHeader
Small ordered header list/map.

- preserves the original display-case key while normalizing the lookup key to lowercase
- `append()` merges a repeated header into the existing entry's value rather than creating a duplicate list entry, so `contains()`/`value()`/iteration always agree on how many times a given header name appears
- `toString()` renders the `"Key: value\r\n"` wire format directly from the ordered list

### JobIana
Lookup table mapping the `IanaHeaders` enum to canonical header name strings and back, plus a `std::formatter` specialization so header names can be used directly in log/format strings.

## Resolution

### JobResolver
Owns DNS lookup, kept deliberately separate from the socket layer. Sockets are never allowed to call `getaddrinfo`/`GetAddrInfoA` themselves.

- `resolveAsync()` runs the blocking lookup on an injected `ThreadPool`, then hops the result back onto the `JobIoAsyncThread` loop thread via `post()` before invoking the caller's callback. Resolution never blocks the shared event loop, and a slow or hanging lookup for one connection can't stall IO for every other socket on that loop
- construction is factory-only (`JobResolver::create(loop, workerPool, proxyConfig = {})`), since `resolveAsync()` relies on `shared_from_this()`
- when a proxy is configured and it declares `ProxyCapability::HostNameLookup`, the resolver redirects resolution to the proxy gateway's address instead of the destination's. This is how proxy-awareness stays out of `TcpSocket`/`UnixSocket` entirely
- a socket or client can be given its own dedicated resolver (and by extension its own worker pool) rather than being forced through one shared instance, if that's ever needed for isolation or prioritization

### JobProxyConfig
Plain configuration struct. `ProxyType`/`ProxyCapability` enums plus host/port. job_net never performs its own proxy discovery; the app layer is responsible for populating this (POSIX env vars, Windows registry/WinHTTP) and injecting it into a `JobResolver`.

## Socket layer

### ISocketIO
Abstract base every concrete socket implements, and the layer where all the genuinely shared, portable connect/bind/event logic lives.

- owns the fd, a `weak_ptr` to `JobIoAsyncThread`, and an optional `JobResolver::Ptr`
- `connectToHost(const JobIpAddr&)` is the one pure-virtual primitive every transport implements. Connecting to an already-resolved address, no DNS involved by definition
- `connectToHost(const JobUrl&)` is a single shared, virtual implementation: it resolves via the injected `JobResolver`, then tries each returned candidate address in turn through the primitive above. Every socket type gets URL-based connect for free instead of reimplementing resolve-then-connect; `UnixSocket` is the one exception, overriding it with its own path-based (non-DNS) logic, since a filesystem path has no hostname concept at all
- `bind(const std::string&, uint16_t)` is a similar shared convenience wrapping `bind(JobIpAddr)`
- `registerEvents()`/`modifyEvents()` wrap `JobIoAsyncThread::registerFD`/`modifyFD`, capturing `weak_from_this()` rather than a raw `this` in the dispatch callback, so a socket destroyed while a callback is already in flight can't be called into after it's gone
- `onConnect`/`onRead`/`onWrite`/`onDisconnect`/`onError`/`onAccept` are the public callback surface every concrete socket and every client/server wrapper hooks into
- `SocketState` distinguishes `Bound` from `Connected`. A socket that's only had `bind()` called has no default peer, which is a real distinction for UDP in particular

### TcpSocket / UdpSocket / UnixSocket
Concrete transports, each a thin, mostly-symmetric implementation over `ISocketIO`, with a Linux and a Windows backend selected at build time.

- construction is factory-only on all three (`TcpSocket::create(...)` etc.) via a private-constructor-plus-passkey pattern that still allows `make_shared` (no extra allocation), since `weak_from_this()`/`shared_from_this()` require the object to already be `shared_ptr`-owned
- all three reject address-family mismatches explicitly (a `Family::Unix` address handed to `TcpSocket`/`UdpSocket`, or an IPv4/IPv6 address handed to `UnixSocket`) rather than silently mishandling the bytes
- `bind(const JobUrl&)` on `TcpSocket`/`UdpSocket` only accepts an already-numeric IPv4/IPv6 literal. Binding to a local interface never has a sensible reason to involve DNS, so this path never touches the resolver. `UnixSocket::bind(JobUrl)` extracts the path directly, same non-DNS reasoning
- `UnixSocket` tracks whether it actually owns its bound path (`m_ownsPath`). An accepted child connection's local address happens to read back as the listener's own path via `getsockname()`, and without this distinction a client disconnecting could unlink the *server's* socket file out from under it
- on Windows, the fd-vs-`SOCKET` boundary is handled by `WinFdReg` (living in `job_threads`, alongside the async IO engine that owns `registerFD`/`unregisterFD`/`modifyFD`, since that's genuinely where the int-token-to-native-handle mapping belongs) rather than by job_net itself

## TLS

### JobSslContext / JobSslError
Platform-pluggable TLS context and error translation. Genuinely two different engines under one shared interface, OpenSSL on Linux and Schannel on Windows, not just an OpenSSL-everywhere implementation with a Windows trust-store shim.

- `loadSystemCertificates()`/`loadCaCertificateFile()`/`loadCertificateFile()`/`loadPrivateKeyFile()` cover the OpenSSL-native PEM/DER-plus-separate-key-file model; `loadIdentityFile()` covers Schannel's native PKCS#12/PFX cert-plus-key-in-one-blob model via `PFXImportCertStore`. The two aren't interchangeable, since a bare certificate context on Windows has no way to carry an associated private key
- `ensureCredentials()` is the one cross-platform "ready for a handshake" signal; `isValid()` means "safely configurable" on both platforms rather than tracking Windows-specific credential-handle staleness
- `JobSslError` normalizes both backends' native error codes into one shared `SslErrNo` enum, with the native-domain conversion (`recordNativeError()`) kept explicitly backend-internal so portable code never has to know whether an `int` came from `SSL_get_error()` or a `SECURITY_STATUS`

`ssl_socket`, the decoupled interceptor filter wrapping an `ISocketIO::Ptr` that actually drives a handshake using the two classes above, is designed but not yet built. It's next in line, along with proxy CONNECT-tunnel support.

## Client/server helpers

Convenience wrappers around a socket plus the async loop. The layer most application code actually touches.

### Clients
`TcpClient`/`UdpClient`/`UnixClient` each own a socket, forward its callbacks, and expose `connectToHost(JobIpAddr)`/`connectToHost(JobUrl)` plus `send()`. `TcpClient`/`UdpClient` can take an optional `JobResolver::Ptr` (constructor or `setResolver()`) for the `JobUrl` path; `UnixClient` never needs one, since `UnixSocket`'s URL overload is path-based. `setSocket()` (`TcpClient`/`UnixClient`) lets a server hand in an already-accepted socket, clearing the outgoing socket's callbacks first so a replaced socket can't keep firing into a client that's moved on.

### Servers
`TcpServer`/`UnixServer` hold a listener socket plus a tracked set of client wrappers; `UdpServer` is a single bound socket with `sendTo`/`recvFrom`, since UDP has no per-connection accept model. All client-tracking callback wiring uses `weak_ptr` captures rather than capturing a client's own `shared_ptr` into its own stored callback, and `stop()` swaps the client list out under a brief lock before disconnecting everyone with the lock released, so a client's own synchronous disconnect callback reentering the server can't deadlock or mutate the list mid-iteration.

## Threading model

DNS resolution, socket IO, and callback delivery are deliberately kept on different sides of a boundary. The `JobIoAsyncThread` loop thread runs `epoll_wait`/`WSAPoll` and delivers every IO callback and every completed resolution's result; blocking work, currently just `JobResolver`'s `getaddrinfo`/`GetAddrInfoA` calls, runs on a separate `ThreadPool` and never touches the loop thread directly.