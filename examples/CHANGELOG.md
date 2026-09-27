# Changelog

## v1.2.1

- Retains the last valid BMS sensor values while the on-demand BLE connection is intentionally disconnected.
- Production on-demand BLE lifecycle: connect, poll, publish, disconnect.
- Validated with repeated 10-minute polling cycles.

## v1.2.0

- Changed the BMS connection lifecycle to on-demand polling: connect, poll, then disconnect.
- Added full-response validation before decoding the 122-register payload.
- Improved BLE service and characteristic discovery.
- Uses ESPHome BLEClient notification registration and lifecycle APIs.
- Added connection lifecycle logging and failed/incomplete poll handling.
- Removed the duplicate manual CCCD write that caused duplicate BMS requests after notification registration.

## v1.1.0

- Added configurable polling interval.
- Added BLE connection binary sensor.
- Sensors become unavailable on disconnect.
- Added BLE notification watchdog.

## v1.0.0

- Initial public release.
- SOC.
- Voltage.
- Current.
- Temperature sensors.
- Cell voltage monitoring.
