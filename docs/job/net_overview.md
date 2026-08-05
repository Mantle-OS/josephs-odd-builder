# Job Net

Cross-platform asynchronous networking for TCP, UDP, Unix-domain sockets, TLS, URL parsing, and HTTP metadata.

`job_net` links against `job_core`, `job_threads`, and `job_crypto`. Linux builds additionally link OpenSSL, libcrypto, and `atomic`. Windows builds link the native Winsock, security, DNS, network-interface, and WinHTTP libraries. It has no Qt dependency.

* `job::net` separates portable networking behavior from Linux and Windows socket implementations
* low-level socket classes expose transport behavior, while client and server wrappers handle application-facing ownership and callback orchestration
* hostname resolution, proxy configuration, TLS policy, and error translation remain separate from the socket implementations that consume them
* asynchronous socket events are driven through `job::threads::JobIoAsyncThread`
* public classes use JOB and standard-library types, while native socket structures remain behind the platform boundary

## Core model

### JobIoAsyncThread integration

Job Net does not maintain a separate networking event loop. Socket implementations register their native handles and requested events with `JobIoAsyncThread`, which reports readable, writable, connection, and error activity through callbacks.

Job Net uses that loop for transport events, deferred work, and posting resolver results back from worker threads. Networking callbacks therefore execute through the I/O loop unless an application explicitly forwards them elsewhere.

On Windows, native handle registration and lookup are owned by `job_threads` through `WinFdReg`. Job Net uses that shared infrastructure rather than maintaining its own Windows socket registry.

See [Job threading overview](docs/job/threading_overview.md) for the complete asynchronous I/O, descriptor registration, and callback execution model.

### ISocketIO

`ISocketIO` is the common transport interface used by stream-oriented Job Net components.

It supplies the shared behavior required by TCP, Unix-domain, and TLS transports:

* connection and disconnection
* reading and writing bytes
* local and peer endpoint information
* transport state and error reporting
* callbacks for connection, incoming data, writable state, disconnection, and failures
* linkage to the `JobIoAsyncThread` driving the transport

Higher-level wrappers can operate through `ISocketIO::Ptr` without depending directly on a concrete socket implementation. `SslSocket` uses this boundary to add encryption around an existing stream transport rather than duplicating TCP or Unix-domain socket behavior.

UDP retains datagram semantics and should not be treated as a byte stream merely for interface uniformity.

### Construction and shared ownership

Network objects whose callback and lifetime behavior requires shared ownership expose a `Ptr` alias and a static `create()` factory.

Classes using `std::enable_shared_from_this` must begin life under a `std::shared_ptr` before they call `shared_from_this()`. Where factory-only construction is enforced, the constructor accepts a private construction tag:

```cpp
struct PrivateTag {
private:
    PrivateTag() = default;
    friend class UnixClient;
};
```

The constructor remains callable by `std::make_shared`, but outside code cannot create the tag and bypass `create()`.

This is an ownership rule rather than a requirement for every Job Net type. Value objects such as addresses and configuration structures can still use ordinary construction.

Client and server wrappers detach callbacks during teardown so pending transport events cannot call back into orchestration state that has already been destroyed.

## Addresses, resolution, and proxy configuration

### JobIpAddr

`JobIpAddr` is the portable endpoint value used between resolvers, sockets, clients, and servers.

It stores one native endpoint inside `sockaddr_storage` while exposing a common interface for:

* IPv4 addresses
* IPv6 addresses
* Unix-domain socket paths
* endpoint ports
* conversion to and from native `sockaddr` values
* endpoint classification and validation
* printable address formatting

The native storage is kept inside the value so it can be passed directly to platform socket calls without repeatedly reparsing the textual address. `sockAddr()` and `sockAddrLen()` expose the stored native representation when an implementation must call `bind()`, `connect()`, `sendto()`, or another operating-system API.

`setAddress()` clears the previous value and then classifies the supplied string as Unix-domain, IPv4, or IPv6. The address becomes valid only after the matching native representation has been constructed successfully.

IPv4 and IPv6 input must be numeric. `JobIpAddr` does not perform hostname resolution; names are handled by `JobResolver`.

`fromSockAddr()` performs the reverse operation. It validates the pointer, length, and address family before copying an accepted, received, or queried native address into the portable object.

The stored family is reported as:

* `Family::IPv4`
* `Family::IPv6`
* `Family::Unix`
* `Family::Unknown`

`clear()` resets the family, port, length, validity flag, and entire native storage.

#### Formatting

`toString()` converts the stored endpoint back into a readable form.

IPv4 endpoints include the port as:

```text
127.0.0.1:8080
```

IPv6 endpoints use brackets when a port is included:

```text
[::1]:8080
```

Unix filesystem paths are returned directly. Linux abstract namespace sockets are displayed with `@` in place of their leading null byte:

```text
@job-server
```

Invalid objects format as `(invalid)`.

#### Address classification

`JobIpAddr` provides classification helpers for common routing and binding decisions:

* `isLocal()` identifies Unix-domain endpoints, IPv4 addresses in `127.0.0.0/8`, IPv6 loopback, and IPv6 link-local addresses
* `isLoopback()` identifies IPv4 `127.0.0.1` and IPv6 `::1`
* `isMulticast()` identifies IPv4 multicast space and IPv6 multicast addresses
* `isNull()` identifies IPv4 `0.0.0.0` and the unspecified IPv6 address
* `isBroadcast()` identifies IPv4 `255.255.255.255`
* `isGlobal()` excludes the private, loopback, link-local, multicast, unspecified, and reserved ranges currently recognized by the implementation

The current IPv4 `isGlobal()` check excludes:

* `0.0.0.0`
* `10.0.0.0/8`
* `127.0.0.0/8`
* `172.16.0.0/12`
* `192.168.0.0/16`
* `169.254.0.0/16`
* addresses beginning at `224.0.0.0`

The IPv6 check excludes unspecified, loopback, link-local, and multicast addresses.

These helpers describe the classifications implemented by Job Net; they are not a complete general-purpose registry of every special-purpose address block.

#### Unix-domain paths

A string is treated as a Unix-domain socket address when it is:

* a Linux abstract namespace value beginning with a null byte
* an absolute path
* a path beginning with `./` or `../`
* a name ending in `.sock`

Filesystem Unix sockets are null-terminated inside `sockaddr_un`. Linux abstract namespace sockets preserve their leading null byte and use the exact native length required by the kernel.

`isUnixPermitted()` checks whether the stored Unix endpoint can currently be used:

* abstract namespace sockets are accepted without a filesystem lookup
* filesystem paths must exist
* the path must identify a socket
* the current process must have read and write access

The maximum path length is limited by the platform `sockaddr_un::sun_path` buffer.

### JobProxyConfig

`JobProxyConfig` describes an explicitly supplied proxy endpoint and its advertised capabilities.

It stores:

* proxy host and port
* proxy protocol type
* a capability bitmask
* optional username and password credentials

Available proxy types are:

* `None`
* `Socks4`
* `Socks5`
* `HttpConnect`
* `HttpCaching`
* `Ftp`

Available capabilities describe intended behavior such as:

* tunneling
* listening
* UDP tunneling
* caching
* proxy-side hostname lookup
* SCTP tunneling
* SSL tunneling

The type and capability values describe configuration intent. They do not themselves implement the corresponding proxy protocol.

`isValid()` accepts `ProxyType::None` without requiring an endpoint. Any configured proxy type requires a non-empty host and a non-zero port.

`supports()` tests whether a required capability is present in the bitmask. The capability enum provides the usual bitwise operations for building and testing combined masks.

Credentials are installed with `setCredentials()` using explicit pointer and length pairs. The username and password are copied into separate `JobSecureMem` objects rather than ordinary strings. Passing a null or empty value clears the corresponding credential.

`clearCredentials()` releases both secure-memory objects.

Job Net does not currently perform automatic proxy discovery from environment variables, Windows settings, or configuration files. Applications create and inject `JobProxyConfig` explicitly.

### JobResolver

`JobResolver` converts host names into `JobIpAddr` endpoints.

It supports synchronous platform resolution and an asynchronous wrapper built from:

* a `JobIoAsyncThread`
* a `ThreadPool`
* an optional `JobProxyConfig`

The blocking platform lookup runs on the supplied worker pool rather than the networking event loop. Once resolution completes, the callback is posted back to the supplied `JobIoAsyncThread`.

The resulting sequence is:

```text
caller
  ↓
resolveAsync()
  ↓
ThreadPool performs resolveSync()
  ↓
JobIoAsyncThread::post()
  ↓
ResolveCallback(errorCode, addresses)
```

The resolver holds only a weak reference to the I/O loop. If the loop is destroyed while a lookup is running, the result is discarded instead of invoking a callback through a dead event loop.

A worker pool is required for asynchronous resolution. `resolveAsync()` fails immediately when no live I/O loop or worker pool is available.

The resolver retains shared ownership of itself while asynchronous work is running, preventing its configuration and platform implementation from disappearing before the worker completes.

#### Host and URL resolution

`resolveAsync()` accepts either:

* an explicit host and port
* a `JobUrl`

When resolving a URL with no explicit port, the resolver supplies:

* port `80` for HTTP
* port `443` for HTTPS

Other URL schemes retain port `0` unless the URL contains a port.

The platform implementation returns every compatible address reported by the native resolver as a `std::vector<JobIpAddr>`.

The current Linux implementation uses `getaddrinfo()` with:

* `AF_UNSPEC`
* `SOCK_STREAM`
* `IPPROTO_TCP`

This permits IPv4 and IPv6 results while requesting endpoints suitable for TCP-style stream connections.

#### Proxy-aware resolution

The resolver stores a mutable `JobProxyConfig`. An asynchronous operation copies that configuration before submitting work to the thread pool, so later calls to `setProxyConfig()` do not alter a lookup already in progress.

Current proxy integration affects only the endpoint selected for DNS resolution.

When a proxy is configured and advertises `HostNameLookup`, the resolver resolves the proxy gateway instead of the requested destination:

```text
requested destination
        ↓
configured proxy has HostNameLookup?
        ├── no  → resolve destination host and port
        └── yes → resolve proxy host and port
```

This provides the address required to connect to the proxy gateway. It does not perform:

* SOCKS4 negotiation
* SOCKS5 negotiation
* HTTP CONNECT
* proxy authentication
* tunnel establishment
* datagram forwarding
* cached request handling

Those protocol stages are not currently implemented by `JobResolver`.

## Errors

### JobSocketError

`JobSocketError` translates platform socket failures into portable Job Net error values.

Linux and Windows implementations interpret their native error codes independently, while higher layers receive the same public Job Net error model.

Transport errors remain separate from TLS errors because a connection can fail at either layer:

```text
native socket or transport failure → JobSocketError
TLS handshake or encrypted I/O failure → JobSslError
```

This allows clients and servers to report whether failure occurred in the underlying connection or in the protocol wrapped around it.

### JobSslError

`JobSslError` represents TLS-specific results independently from ordinary transport failures.

It distinguishes states including:

* no error
* another readable event is required
* another writable event is required
* connection or accept progress is required
* clean TLS closure
* native system-call failure
* TLS handshake failure
* certificate verification failure
* invalid internal state
* unsupported operation
* internal or unknown failure

The currently nonfatal results are:

* `None`
* `WantRead`
* `WantWrite`
* `WantConnect`
* `WantAccept`
* `ZeroReturn`

`ZeroReturn` represents orderly TLS shutdown and is not treated as a fatal protocol error.

## TCP

### TcpSocket

`TcpSocket` is the low-level asynchronous TCP transport.

It owns one native stream socket and integrates it with `JobIoAsyncThread`. Its responsibilities include:

* creating and configuring the native socket
* binding to a local endpoint
* listening for incoming connections
* accepting connected transports
* connecting to a remote `JobIpAddr`
* reading and writing stream data
* exposing local and peer endpoint information
* reporting connection, data, disconnection, and socket errors
* changing the I/O events requested from the asynchronous loop
* closing the native transport exactly once

Linux and Windows implementations use their respective socket APIs while preserving the same public behavior.

Applications can use `TcpSocket` directly when they need explicit control over binding, accepting, socket callbacks, or transport lifetime.

### TcpClient

`TcpClient` is the application-facing wrapper for one outbound TCP connection.

It handles orchestration that would otherwise be repeated by each application:

* resolver integration
* transport creation
* callback installation and forwarding
* connection state
* incoming message delivery
* error reporting
* disconnection and teardown

A connected `TcpClient` represents an established TCP transport. It does not imply that a higher-level protocol layered over that transport has completed its own setup.

### TcpServer

`TcpServer` owns a listening TCP socket and the clients accepted from it.

It converts low-level accept events into managed client objects, installs callbacks, tracks active clients, forwards incoming data and errors, and removes clients when they disconnect.

Stopping the server closes the listener and disconnects the clients currently owned by the server.

The wrapper is intended for ordinary application use. Direct `TcpSocket` use remains available when an application requires custom accepted-socket ownership or protocol handling.

## Unix-domain sockets

### UnixSocket

`UnixSocket` provides local stream communication through Unix-domain socket addresses.

Its transport behavior follows the same asynchronous stream model as `TcpSocket`, but addressing uses a local path or abstract namespace name rather than an IP address and port.

The platform implementation handles:

* native Unix-domain socket creation
* binding and connecting by local path
* accepted local transports
* path-length limits
* filesystem socket cleanup where required
* platform support differences

### UnixClient

`UnixClient` wraps one outbound Unix-domain socket connection.

It supplies application-facing connection, message, error, and disconnection callbacks while connecting to a local endpoint rather than a resolved network address.

`UnixClient` uses the private construction-tag pattern so callers create it through `create()` and it begins life under valid shared ownership.

### UnixServer

`UnixServer` listens on a Unix-domain endpoint and owns the accepted local clients created from incoming connections.

It handles listener setup, accepted-client tracking, callback forwarding, disconnection, and local socket cleanup so applications do not need to reproduce that lifecycle.

## TLS

### JobSslContext

`JobSslContext` owns the configuration and native context used to create TLS sessions.

Its responsibilities include the TLS policy and credentials shared by one or more encrypted sockets, such as:

* client or server mode
* certificates
* private keys
* trusted certificate sources
* peer verification behavior
* native TLS context initialization and lifetime

TLS sockets receive an already configured context. They do not independently search for credentials or define application verification policy.

Platform-specific context setup remains behind the same public class.

### SslSocket

`SslSocket` wraps an existing `ISocketIO` stream transport and adds TLS.

The wrapped transport continues to own:

* the native connection
* local and peer addressing
* readable and writable events
* connection and disconnection events
* raw transport lifetime

`SslSocket` owns:

* the native TLS session
* handshake state
* encrypted reads and writes
* TLS error translation
* orderly TLS shutdown

This separates encryption from TCP or Unix-domain socket mechanics.

#### Connection states

Transport connection and TLS readiness are separate phases:

```text
underlying transport created
        ↓
transport connecting
        ↓
transport connected
        ↓
TLS handshake
        ↓
encrypted application connection
```

A connected transport is not yet an application-ready TLS socket.

Readable and writable transport events advance the handshake when the TLS implementation reports that more input or output is required. `onEncrypted` fires only after the handshake completes successfully.

#### Reading and writing

Application writes are accepted only after the TLS session is encrypted.

Incoming transport events are passed through the TLS engine. Decrypted application data is then delivered through the socket's message callback.

TLS results such as `WantRead` and `WantWrite` are progress states rather than fatal failures. They cause the socket to wait for the matching transport event and continue the operation later.

#### Shutdown and native-session ownership

TLS shutdown is distinct from immediately destroying the underlying transport.

`SslSocket` coordinates the close sequence so that:

* only one caller begins shutdown
* orderly TLS closure can be attempted
* transport callbacks remain attached while still required
* the native TLS session is released exactly once
* transport disconnection completes the wrapper state transition
* callback detachment remains separate from TLS-session release

`releaseSsl()` releases only the native TLS session. It does not implicitly detach transport callbacks.

`detachSocketCallbacks()` removes callbacks installed on the wrapped transport.

Keeping those operations separate prevents teardown from removing callbacks that are still needed to finish shutdown.

### SslClient

`SslClient` is the application-facing wrapper for one outbound encrypted connection.

It handles:

* creating or receiving the underlying transport
* resolver configuration
* transport callback installation
* TCP connection
* TLS handshake startup
* encrypted message delivery
* socket and TLS error forwarding
* disconnection and teardown

For `SslClient`, `isConnected()` means the TLS handshake has completed and the socket is encrypted. A transport that has only completed TCP connection is not reported as a fully connected SSL client.

`onConnect` and `onEncrypted` are delivered after successful encryption rather than reporting an application-ready connection before TLS is available.

### SslServer

`SslServer` listens for incoming stream connections and upgrades each accepted transport with a configured server `JobSslContext`.

It owns the accepted encrypted clients and handles:

* listener creation and startup
* accepted transport conversion
* TLS socket and client construction
* callback installation and forwarding
* active-client tracking
* client removal after disconnection
* socket and TLS error reporting
* server-wide shutdown

A valid SSL context is required before the listener can start.

Stopping the server:

* detaches listener callbacks
* disconnects the listener
* removes the active client list under synchronization
* detaches client callbacks that capture server state
* disconnects clients outside the ownership lock
* prevents later transport events from re-entering destroyed server state

Fatal TLS errors are reported independently from ordinary socket failures.

## UDP

### UdpSocket

`UdpSocket` provides asynchronous datagram transport.

Unlike TCP and Unix-domain streams, UDP does not create an accepted connection for each peer and does not provide a persistent ordered byte stream.

Each datagram retains:

* its own message boundary
* its source or destination endpoint
* independent delivery behavior

The platform implementations handle native datagram APIs, socket options, addressing, and error conversion while exposing portable Job Net types.

### UdpClient / UdpServer

The UDP wrappers provide application-level setup, endpoint ownership, callback installation, and message forwarding around `UdpSocket`.

Their names describe the application's role rather than TCP-style connection establishment. They do not make UDP reliable, ordered, or stream-oriented.

## URL and HTTP data types

### JobUrl

`JobUrl` parses and stores URL components without performing network I/O.

It separates URL interpretation from transport code so clients, resolvers, and protocol handlers can consume structured values rather than repeatedly parsing raw strings.

Depending on the URL, it provides values such as:

* scheme
* host
* port
* path
* query
* credentials or related authority information

`JobResolver` can consume a `JobUrl` directly and applies the conventional HTTP or HTTPS port when one is not explicitly present.

### JobHttpHeader

`JobHttpHeader` stores HTTP fields while preserving their stream order.

It supports indexed and name-based operations without reducing the header collection to a simple map. This matters because HTTP can contain repeated fields and because preserving received or generated order is useful during parsing, forwarding, signing, and diagnostics.

Invalid indexed removal or replacement operations are rejected and reported rather than silently affecting another entry.

### JobIana

`JobIana` centralizes protocol names and string mappings used by Job Net.

It prevents clients, servers, and parsers from independently duplicating standardized HTTP or network field names and their conversion behavior.

## Platform boundary

Public Job Net interfaces use portable JOB and standard-library types where practical. Native socket APIs and operating-system behavior remain in platform-specific implementations.

`JobIpAddr` is the main intentional exception at the lowest address boundary: it stores a native `sockaddr_storage` and can expose a `sockaddr` pointer for direct system calls. The family, port, validity, formatting, and classification API around that storage remains portable.

### Linux

Linux builds link:

* OpenSSL
* libcrypto
* `atomic`
* the common JOB libraries and system thread support

The socket implementations use the native Linux networking APIs and the asynchronous descriptor support provided by `job_threads`.

### Windows

Windows builds link:

* `ws2_32`
* `advapi32`
* `secur32`
* `dnsapi`
* `iphlpapi`
* `winhttp`
* the common JOB libraries and system thread support

`NOMINMAX` is defined privately for the Job Net target to prevent Windows `min` and `max` macros from interfering with the C++ standard library.

Native Windows socket registration and handle preservation are owned by `job_threads` through `WinFdReg`. Job Net does not maintain a duplicate socket registry.
