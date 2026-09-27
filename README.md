# ESPHome Eco Battery

ESPHome external component for monitoring compatible Eco Battery LiFePO4 golf-cart batteries through the battery management system (BMS) Bluetooth interface.

The component connects to the BMS over BLE, requests the battery data, publishes the readings to ESPHome/Home Assistant, and then intentionally disconnects. By default, the BMS is polled every 10 minutes.

## Current stable version

**v1.2.1**

The v1.2.1 release is the tested on-demand BLE connection lifecycle:

1. Wait for the configured polling interval.
2. Connect to the BMS.
3. Discover the BMS services and characteristics.
4. Register for BMS notifications.
5. Send the BMS request.
6. Receive and validate the complete response.
7. Publish the battery readings.
8. Disconnect from the BMS.
9. Keep the last valid readings in Home Assistant until the next successful poll.

The component does **not** maintain a permanent BLE connection.

## Features

- BLE communication with the Eco Battery BMS.
- On-demand BLE connection and polling.
- Automatic disconnect after a completed poll.
- Configurable polling interval.
- State-of-charge (SOC).
- Battery voltage.
- Battery current.
- MOS temperature.
- Cell temperature.
- Maximum cell voltage.
- Minimum cell voltage.
- Cell voltage delta.
- Cell count.
- Three temperature probe readings.
- BLE connection status.
- Full-response validation before decoding BMS data.
- Failed or incomplete polls do not intentionally erase the last valid sensor values.

## Requirements

You will need:

- A compatible Eco Battery LiFePO4 battery with the supported BMS Bluetooth interface.
- An ESP32 device supported by ESPHome.
- ESPHome installed and working in Home Assistant.
- The ESP32 within BLE range of the battery.
- The Bluetooth MAC address of the Eco Battery BMS.
- Wi-Fi access for the ESP32.
- Your ESPHome API encryption key, if you use encrypted Home Assistant API communication.

This component uses ESPHome's `esp32_ble_tracker` and `ble_client` components.

## Installation — step by step

This section is intended for users who are not familiar with ESPHome external components.

### 1. Make sure ESPHome is installed

In Home Assistant, open the **ESPHome** add-on/dashboard and make sure you can create and install an ESPHome device.

If you already have ESPHome devices running, you can proceed.

### 2. Create a new ESPHome device

In the ESPHome dashboard:

1. Select **New Device**.
2. Give the device a name, such as `golf-cart-battery`.
3. Select the ESP32 hardware you are using.
4. Complete the initial ESPHome device creation.
5. Open the device's YAML configuration.

The exact ESP32 board setting depends on the hardware you purchased. The example in this repository uses `esp32dev`; change that if your board requires a different ESPHome board definition.

### 3. Start with the complete example YAML

Use the complete example configuration in:

`examples/golf-cart-battery.yaml`

You can open the example directly from the repository and copy the complete configuration into your ESPHome device YAML.

Do **not** copy only the `eco_battery:` section. The complete example includes the ESP32, Wi-Fi, Home Assistant API, OTA, BLE tracker, BLE client, Eco Battery component, and all of the sensors required by the component.

### 4. Update the Eco Battery BMS MAC address

Find:

```yaml
ble_client:
  - mac_address: "***Update me***"
    id: eco_battery_client
    auto_connect: false
```

Replace `***Update me***` with the Bluetooth MAC address of your Eco Battery BMS.

For example:

```yaml
mac_address: "AA:BB:CC:DD:EE:FF"
```

The MAC address must be the BMS, not the ESP32.

### 5. Update the ESPHome API encryption key

Find:

```yaml
api:
  encryption:
    key: "***Update me***"
```

Replace the placeholder with your ESPHome API encryption key.

If your ESPHome configuration uses the normal `secrets.yaml` system, the preferred configuration is:

```yaml
api:
  encryption:
    key: !secret api_encryption_key
```

and the corresponding key is stored in your ESPHome `secrets.yaml`.

Do not publish your real API encryption key in a public repository or forum post.

### 6. Configure Wi-Fi

The example uses ESPHome secrets:

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
```

If you use an ESPHome `secrets.yaml` file, make sure it contains the appropriate values.

If you do not use a secrets file, replace the two values with your Wi-Fi credentials:

```yaml
wifi:
  ssid: "Your Wi-Fi Network"
  password: "Your Wi-Fi Password"
```

Do not publish your real Wi-Fi password.

### 7. Check the ESP32 board

The example currently uses:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
```

Change `esp32dev` if your ESP32 board requires another ESPHome board definition.

The component has been tested with an ESP32 using the ESP-IDF framework.

### 8. Check the external component version

The example uses the stable release:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jayg37/esphome-eco-battery
      ref: v1.2.1
    components: [eco_battery]
```

Using the release tag instead of `main` means your configuration stays on the tested version until you intentionally upgrade it.

### 9. Choose the polling interval

The example uses:

```yaml
eco_battery:
  ble_client_id: eco_battery_client
  update_interval: 10min
```

The BMS is connected only when a poll is due.

For example, `5min` polls every five minutes and `30min` polls every thirty minutes.

A shorter interval means more frequent BLE connections. The default example uses 10 minutes.

### 10. Validate the configuration

In ESPHome, select **Validate**.

Fix any configuration errors before attempting installation.

The most common items to check are:

- BMS MAC address.
- Wi-Fi credentials.
- API encryption key.
- ESP32 board type.
- YAML indentation.

### 11. Install the ESP32

Install the configuration to the ESP32 using the ESPHome dashboard.

For the first installation, USB installation may be required depending on your ESP32 hardware. After the device is running on Wi-Fi, future ESPHome updates can normally be performed over-the-air.

### 12. Watch the ESPHome logs

Open the device logs after installation.

A successful poll should show the ESP32:

- Waiting for the polling interval.
- Connecting to the BMS.
- Discovering the BMS services/characteristics.
- Registering notifications.
- Sending the BMS request.
- Receiving the complete response.
- Publishing the readings.
- Disconnecting from the BMS.

The BMS should not remain connected between polls.

### 13. Check Home Assistant

After the ESPHome device connects to Home Assistant, the Eco Battery entities will be available from the ESPHome device.

The sensor values are retained between successful polls. During the intentional period when the ESP32 is disconnected from the BMS, Home Assistant continues to show the most recently received valid readings.

The **Eco Battery Connected** binary sensor indicates whether the ESP32 currently has an active BMS connection.

## Complete configuration example

The repository includes a complete working configuration in:

`examples/golf-cart-battery.yaml`

The example contains:

- ESPHome device definition.
- Git-based external component.
- ESP32/ESP-IDF configuration.
- Logger.
- Home Assistant API.
- OTA.
- Wi-Fi.
- BLE tracker.
- BLE client.
- Eco Battery component.
- All supported sensor definitions.
- BMS connection status.

Before using it, update every value marked `***Update me***`.

## Configuration reference

### External component

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jayg37/esphome-eco-battery
      ref: v1.2.1
    components: [eco_battery]
```

### BLE client

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: "***Update me***"
    id: eco_battery_client
    auto_connect: false
```

`auto_connect: false` is intentional. The Eco Battery component controls the connection lifecycle and connects only when a poll is due.

### Eco Battery component

```yaml
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

## Sensors

The component provides the following data:

| Entity | Description |
|---|---|
| Eco Battery SOC | Battery state of charge |
| Eco Battery Voltage | Battery voltage |
| Eco Battery Current | Battery current |
| Eco Battery MOS Temperature | BMS/MOS temperature |
| Eco Battery Cell Temperature | Cell temperature |
| Eco Battery Max Cell Voltage | Highest reported cell voltage |
| Eco Battery Min Cell Voltage | Lowest reported cell voltage |
| Eco Battery Cell Delta | Difference between maximum and minimum cell voltage |
| Eco Battery Cell Count | Number of cells reported by the BMS |
| Eco Battery Temp Probe 1 | BMS temperature probe 1 |
| Eco Battery Temp Probe 2 | BMS temperature probe 2 |
| Eco Battery Temp Probe 3 | BMS temperature probe 3 |
| Eco Battery Connected | Current BLE connection status |

## BLE connection behavior

The component intentionally does not maintain a permanent GATT connection.

Each polling cycle is:

```text
IDLE
  ↓
Connect
  ↓
Discover services
  ↓
Discover characteristics
  ↓
Register notifications
  ↓
Send BMS request
  ↓
Receive response
  ↓
Validate response
  ↓
Decode and publish values
  ↓
Disconnect
  ↓
IDLE
```

A normal disconnect after a successful poll is expected behavior.

The last valid sensor values are retained while the BMS is disconnected.

## Troubleshooting

### All entities exist but show Unknown

Check the ESPHome logs first.

If the BMS response is successfully received and decoded, the BLE communication is working. The component is designed to retain the last valid readings between intentional polling disconnects.

### The ESP32 cannot find the BMS

Check:

1. The BMS Bluetooth MAC address.
2. That the battery/BMS is powered and awake.
3. That the ESP32 is within BLE range.
4. ESPHome logs for BLE discovery messages.
5. That another device is not preventing the BMS from advertising or accepting a connection.

### The device connects but no values are published

Check the logs for:

- Service discovery.
- Characteristic discovery.
- Notification registration.
- BMS request transmission.
- Complete response reception.

An incomplete or invalid BMS response is not decoded as a valid reading.

### The device repeatedly disconnects

The component intentionally disconnects after a poll. A disconnect immediately after a successful response is normal.

### I want more frequent readings

Reduce `update_interval`, for example:

```yaml
update_interval: 5min
```

Remember that the component creates a new BLE connection for each poll.

## Versioning

This project uses semantic version tags for stable releases.

- Stable releases use tags such as `v1.2.1`.
- The example configuration references a specific stable release.
- Changes should be tested before the release tag is updated.
- Users can remain on a known stable release until they intentionally update the `ref:` value.

### Release history

#### v1.2.1

- Retains the last valid BMS sensor values during intentional BLE disconnects.
- Production on-demand BLE polling lifecycle.
- Successful repeated field testing of connect → poll → disconnect operation.

#### v1.2.0

- Changed the BMS connection lifecycle to on-demand polling.
- Added complete-response validation.
- Improved BLE service and characteristic discovery.
- Uses ESPHome BLE notification registration and lifecycle APIs.
- Improved connection lifecycle logging and failed/incomplete poll handling.

#### v1.1.0

- Added configurable polling interval.
- Added BLE connection binary sensor.
- Added BLE notification watchdog.

#### v1.0.0

- Initial public release.
- Added SOC, voltage, current, temperature, and cell-voltage monitoring.

## Development

The `main` branch is the production branch.

Development changes should be tested before being merged into `main` and released under a new version tag.

## Security and privacy

Do not publish:

- ESPHome API encryption keys.
- Wi-Fi passwords.
- Other ESPHome secrets.

The example configuration intentionally uses `***Update me***` placeholders for values that must be supplied by the individual user.

## License

See the repository for the current project license.
