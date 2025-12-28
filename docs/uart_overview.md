# Job UART

serial of job.

jobuart links against `job_core`, `job_threads`, and `libudev`.

- `job::core` defines `job::core::IODevice`
- `job::threads` defines `job::threads::JobIoAsyncThread` (epoll loop used for async FD callbacks)
- `job::uart` is serial port discovery + settings + an `IODevice` implementation

## Serial settings (`SerialSettings`)

Small termios settings bundle:
- port name
- baud rate
- data bits
- parity
- stop bits
- flow control

Includes:
- conversion helpers into termios flags
- lists of available options (bits/parity/stop/flow)
- string formatting for current settings

## SerialIO

`job::core::IODevice` implementation for a serial port.
- opens a `/dev/tty*` path
- applies `SerialSettings` to termios (baud, bits, parity, stop, flow)
- uses nonblocking fd reads
- uses an internal `job::threads::JobIoAsyncThread` to watch the fd (epoll)
- delivers incoming bytes through a read callback
- optional recording: incoming bytes mirrored to a log file

State tracked inside the object:
- connection state (disconnected/connected/busy)
- error state + error string
- device metadata fields (description/manufacturer/serial/vendorId/productId/location)
- upload percent (used as a generic progress marker)

## Serial discovery (`serial_info`)
- filesystem scan for likely serial ports (by name prefix)
- udev scan that fills in metadata using `libudev`:
  - model/vendor/serial
  - vendorId/productId
  - resolved device node path (`/dev/...`)

Discovery output is a map keyed by device path, holding `SerialIO` objects.

## SerialManager

Manager for the device map + current selection.
- map of known devices (path -> `SerialIO`)
- current device pointer
- a shared `JobIoAsyncThread` used for udev monitoring
- initial scan on construction
- rescans on udev add/remove events
- keeps `currentDevice` set to “first device” if none selected and something exists

## UdevMonitorThread

udev listener:
- creates a udev monitor for `tty`
- registers the udev fd into a `JobIoAsyncThread`
- on `add`/`remove` actions, triggers a callback (used by `SerialManager` to rescan)
