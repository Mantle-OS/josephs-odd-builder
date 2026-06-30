# Job Net

The network of job.

jobnet links against `job_core`, `job_threads`, and `job_crypto`.

- `job::threads` provides `job::threads::JobIoAsyncThread` (epoll loop used to drive socket readiness)
- `job::net`: url/ip helpers + socket wrappers + client/server helpers


## Utilities

### JobUrl
URL parser + container.
- scheme (tcp/udp/unix/http/https/etc)
- host + port
- path/query/fragment
- optional user/password storage (password stored in secure memory)

connection description for the socket layer.

### JobIpAddr
Address container for:
- IPv4
- IPv6
- Unix socket path

Stores sockaddr + length + family + port and tracks validity.

### JobHttpHeader
Small header list/map:
- preserves display key casing
- normalizes lookup keys (lowercase)
- supports repeated set/update behavior
- string output for “header: value” format

### JobIana
lookup table for common names named ports  protocol helpers

## Socket layer

### ISocketIO
Common shape for sockets with an fd behind them.
- an fd
- a weak ref to `JobIoAsyncThread`
- state + error tracking hooks

loop:
- fd gets registered for epoll events
- events get forwarded into the socket object
- concrete sockets decide what to do with those events

Socket “type” includes TCP/UDP/UNIX/SSL (SSL is placeholder).

### TcpSocket
TCP socket wrapper:
- connect/bind/listen/accept
- read/write on the connected fd
- peer/local address tracking
- write path has a mutex

### UdpSocket
UDP socket wrapper:
- bind/connect style usage
- read/write via datagrams
- keeps state and error tracking 

### UnixSocket
Unix domain socket wrapper:
- stream unix socket lifecycle
- path-based addressing via `JobIpAddr` (family=Unix)
- used by unix client/server helpers

## Client/server helpers

Wrappers around sockets + the async loop.

### Clients
- TCP client: connect + send + async read callback
- UDP client: send/receive wrapper around UDP socket
- Unix socket client: connect + send + async read callback

### Servers
- TCP server: listener socket + accept -> creates client objects for connections
- UDP server: bind + receive callback
- Unix socket server: listener + accept -> creates unix client objects

