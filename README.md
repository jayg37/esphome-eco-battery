# esphome-eco-battery
ESPHome BLE component for Eco Battery LiFePO4 golf cart batteries.
Overview
BLE integration for Eco Battery LiFePO4 batteries using ESPHome. This custom component allows ESPHome to pull in information from your local Eco battery BMS powered golf cart. I tested this on my 2026 Epic E60X with a 105AH ECO Battery. The ESP32-S3 is roughly 4 foot from the parking area of my golf cart and seems to have zero issues picking up the data. (Be careful with cheap knock off ESP devices as their BLE is weak and the ECO Battery BMS system uses a weak BLE signal as well.Add both of those together and the device will have to be sitting right on top of the golf cart battery...learned from some frustrating testing experience.

Tested with:
- Eco Battery 48V 105AH Golf Cart Battery
- ESP32-S3
- ESPHome 2026.5+.

## Features
```
- State of Charge (SOC)
- Pack Voltage
- Pack Current
- MOS Temperature
- Cell Temperature
- Cell Count
- Maximum Cell Voltage
- Minimum Cell Voltage
- Cell Voltage Delta
- Temperature Probes 1-3
- BLE Connection Status Sensor
- Configurable Update Interval
```
Requirements
ESP32
ESPHome 2026.5+
Bluetooth enabled

## Installation
Install is easy. Commision your new ESP device via ESP web (https://web.esphome.io/?dashboard_wizard) or your local ESPHome instance or IDE..etc. Then when you've got the new device to show up in your ESPHome instance simply edit the device and copy/paste the device yaml below. Click install and it will pull the necessary components from github then compile and install on your device. You're now ready to go! 
Remove components you are not interested in seeing from the sensor lists below. Additionally the interval at which data is pulled from your BMS can be adjusted via the yaml below. Look for "update_interval: 30s" and modify to your needs. 30s, 1min, 5min, 10min, 1hr

## Example yaml for ESP device
```yaml
esphome:
  name: golf-cart-battery1
  friendly_name: Golf Cart Battery 1
  min_version: 2026.4.0
  name_add_mac_suffix: false

external_components:
  - source:
      type: git
      url: https://github.com/jayg37/esphome-eco-battery
      ref: v1.0.0 

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf

logger:
  level: WARN

api:

ota:
  - platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

esp32_ble_tracker:

ble_client:
  - mac_address: 10:23:81:BC:04:87
    id: eco_battery_client

eco_battery:
  ble_client_id: eco_battery_client
  update_interval: 30s
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

sensor:
  - platform: template
    id: eco_soc
    name: "Eco Battery SOC"
    unit_of_measurement: "%"
    device_class: battery
  - platform: template
    id: eco_voltage
    name: "Eco Battery Voltage"
    unit_of_measurement: "V"
    device_class: voltage
  - platform: template
    id: eco_mos_temp
    name: "Eco Battery MOS Temperature"
    unit_of_measurement: "°C"
    device_class: temperature
  - platform: template
    id: eco_cell_temp
    name: "Eco Battery Cell Temperature"
    unit_of_measurement: "°C"
    device_class: temperature
  - platform: template
    id: eco_current
    name: "Eco Battery Current"
    unit_of_measurement: "A"
    device_class: current
  - platform: template
    id: eco_max_cell_voltage
    name: "Eco Battery Max Cell Voltage"
    unit_of_measurement: "V"
    device_class: voltage
  - platform: template
    id: eco_min_cell_voltage
    name: "Eco Battery Min Cell Voltage"
    unit_of_measurement: "V"
    device_class: voltage
  - platform: template
    id: eco_cell_delta
    name: "Eco Battery Cell Delta"
    unit_of_measurement: "V"
    icon: mdi:battery-sync
  - platform: template
    id: eco_cell_count
    name: "Eco Battery Cell Count"
    icon: mdi:battery-outline
  - platform: template
    id: eco_temp_probe1
    name: "Eco Battery Temp Probe 1"
    unit_of_measurement: "°C"
    device_class: temperature
  - platform: template
    id: eco_temp_probe2
    name: "Eco Battery Temp Probe 2"
    unit_of_measurement: "°C"
    device_class: temperature
  - platform: template
    id: eco_temp_probe3
    name: "Eco Battery Temp Probe 3"
    unit_of_measurement: "°C"
    device_class: temperature

binary_sensor:
  - platform: template
    id: eco_connected
    name: "Eco Battery Connected"
```
examples/golf-cart-battery.yaml
## Example Home Assistant Lovelace Dashboard Card yaml
```yaml
show_name: true
show_icon: true
show_state: true
type: glance
entities:
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_soc
    name: Charge
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_voltage
    name: Voltage
    icon: mdi:flash-triangle
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_current
    name: Current
    icon: mdi:current-dc
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_cell_count
    name: Cell Count
    icon: mdi:pound
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_max_cell_voltage
    name: Max Cell V
    icon: mdi:flash-triangle
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_min_cell_voltage
    name: Min Cell V
    icon: mdi:flash-triangle
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_cell_delta
    name: Delta Cell V
    icon: mdi:delta
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_cell_temperature
    name: Cell Temp
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_mos_temperature
    name: MOS Temp
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_temp_probe_1
    name: Temp Probe 1
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_temp_probe_2
    name: Temp Probe 2
  - entity: sensor.garage_golf_cart_battery_1_eco_battery_temp_probe_3
    name: Temp Probe 3
  - entity: binary_sensor.garage_golf_cart_battery_1_eco_battery_connected
grid_options:
  columns: 48
  rows: auto
title: Golf Cart Data
```
## Example Home Assistant Automations
```yaml
alias: Golf Cart Battery Critical Cell Imbalance
description: Critical battery imbalance detected
triggers:
  - trigger: numeric_state
    entity_id: sensor.garage_golf_cart_battery_1_eco_battery_cell_delta
    above: 0.1
actions:
  - action: notify.mobile_app_jeremy_phone
    data:
      title: Golf Cart Battery CRITICAL Alert
      message: >
        Critical cell imbalance detected. Delta is currently {{
        states('sensor.garage_golf_cart_battery_1_eco_battery_cell_delta') }}V.
mode: single
```
```yaml
alias: Golf Cart Charge Reminder
description: Remind me to charge the golf cart if it is in the garage and not charging.
triggers:
  - trigger: time
    at: "20:00:00"
conditions:
  - condition: state
    entity_id: binary_sensor.garage_golf_cart_battery_1_eco_battery_connected
    state: "on"
  - condition: numeric_state
    entity_id: sensor.garage_golf_cart_battery_1_eco_battery_soc
    below: 95
  - condition: numeric_state
    entity_id: sensor.garage_golf_cart_battery_1_eco_battery_current
    above: -1
actions:
  - action: notify.mobile_app_your_phone
    data:
      title: Golf Cart Charging Reminder
      message: >
        The golf cart is in the garage with a charge level of {{
        states('sensor.garage_golf_cart_battery_1_eco_battery_soc') }}%. It does
        not appear to be charging. Plug it in before bed.
mode: single
```
## Supported Registers

| Register   | Meaning       |
| ---------- | ------------- |
| REG[02]    | Cell Count    |
| REG[04]    | Current       |
| REG[05]    | SOC           |
| REG[22]    | MOS Temp      |
| REG[23]    | Temp Probe 1  |
| REG[24]    | Temp Probe 2  |
| REG[25]    | Temp Probe 3  |
| REG[33-48] | Cell Voltages |
