# esphome-eco-battery

ESPHome BLE component for Eco Battery LiFePO4 golf cart batteries.

## Development status

The public main branch is the stable published version. Experimental changes are developed and field-tested on a separate branch before being proposed for merge.

Current development work is in pull request #1.

## BLE connection behavior

The development version changes the BMS connection lifecycle from a persistent GATT connection to an on-demand poll:

1. Wait for the configured update interval.
2. Connect to the Eco Battery BMS.
3. Discover services.
4. Register notifications.
5. Send the Modbus request.
6. Receive and validate the complete response.
7. Publish the register data.
8. Disconnect from the BMS.

This keeps the existing register map and Modbus request while avoiding a permanent GATT session.

The default update interval remains 10min. This is intentionally conservative for the first field-testing phase because the existing implementation was already proven to decode the BMS correctly at that interval.

## Reliability improvements

The development branch also:

- Validates that a response contains the full 122-register payload before decoding.
- Uses the service UUID plus characteristic UUID to locate the write characteristic.
- Logs poll start, request writes, notification sizes, response completion, and disconnect behavior.
- Disconnects cleanly after a successful response or an incomplete/failed poll.
- Uses ESPHome 2026.9's BLEClient connect() and disconnect() lifecycle.
- Uses BLEClient::register_for_notify() rather than directly issuing the registration call, so ESPHome can correctly track pending notification registrations.

## Installation

Use the stable main branch for normal/public installations.

For field testing, pin external_components to the development branch:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jayg37/esphome-eco-battery
      ref: dev/connection-lifecycle
```

## Example test configuration

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: 10:23:81:BC:04:87
    id: eco_battery_client
    auto_connect: false

eco_battery:
  ble_client_id: eco_battery_client
  update_interval: 10min
  connected_sensor: eco_connected
  soc_sensor: eco_soc
  voltage_sensor: eco_voltage
  mos_temp_sensor: eco_mos_temp
  cell_temp_sensor: eco_cell_temp
  current_sensor: eco_current
  max_cell_voltage_sensor: eco_max_cell_voltage
  min_cell_voltage_sensor: eco_min_cell_voltage
  cell_delta_sensor: eco_cell_delta
  cell_count_sensor: eco_cell_count
  temp_probe1_sensor: eco_temp_probe1
  temp_probe2_sensor: eco_temp_probe2
  temp_probe3_sensor: eco_temp_probe3
```

The development branch should not be considered ready for general publication until field testing confirms repeated connect -> poll -> disconnect cycles and recovery after the BMS has been idle.
