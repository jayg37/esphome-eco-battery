import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import ble_client
from esphome.components import sensor
from esphome.components import binary_sensor

from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

DEPENDENCIES = ["ble_client"]

eco_battery_ns = cg.esphome_ns.namespace("eco_battery")

EcoBattery = eco_battery_ns.class_(
    "EcoBattery",
    cg.Component,
    ble_client.BLEClientNode,
)

CONF_BLE_CLIENT_ID = "ble_client_id"
CONF_SOC_SENSOR = "soc_sensor"
CONF_VOLTAGE_SENSOR = "voltage_sensor"
CONF_MOS_TEMP_SENSOR = "mos_temp_sensor"
CONF_CELL_TEMP_SENSOR = "cell_temp_sensor"
CONF_CURRENT_SENSOR = "current_sensor"
CONF_MAX_CELL_VOLTAGE_SENSOR = "max_cell_voltage_sensor"
CONF_MIN_CELL_VOLTAGE_SENSOR = "min_cell_voltage_sensor"
CONF_CELL_DELTA_SENSOR = "cell_delta_sensor"
CONF_CELL_COUNT_SENSOR = "cell_count_sensor"
CONF_TEMP_PROBE1_SENSOR = "temp_probe1_sensor"
CONF_TEMP_PROBE2_SENSOR = "temp_probe2_sensor"
CONF_TEMP_PROBE3_SENSOR = "temp_probe3_sensor"
CONF_CONNECTED_SENSOR = "connected_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(EcoBattery),

        cv.Required(CONF_BLE_CLIENT_ID):
            cv.use_id(ble_client.BLEClient),
        cv.Optional(CONF_SOC_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_VOLTAGE_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_MOS_TEMP_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_CELL_TEMP_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_CURRENT_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_MAX_CELL_VOLTAGE_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_MIN_CELL_VOLTAGE_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_CELL_DELTA_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_CELL_COUNT_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_TEMP_PROBE1_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_TEMP_PROBE2_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_TEMP_PROBE3_SENSOR):
            cv.use_id(sensor.Sensor),
        cv.Optional(CONF_UPDATE_INTERVAL,default="10min"):
            cv.update_interval,
        cv.Optional(CONF_CONNECTED_SENSOR):
            cv.use_id(binary_sensor.BinarySensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):

    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)

    parent = await cg.get_variable(
        config[CONF_BLE_CLIENT_ID]
    )
    cg.add(parent.register_ble_node(var))
    cg.add(
        var.set_update_interval(
            config[CONF_UPDATE_INTERVAL].total_milliseconds
        )
    )

    if CONF_SOC_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_SOC_SENSOR]
        )
        cg.add(
            var.set_soc_sensor(sens))
        
    if CONF_VOLTAGE_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_VOLTAGE_SENSOR]
        )
        cg.add(var.set_voltage_sensor(sens))
        
    if CONF_MOS_TEMP_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_MOS_TEMP_SENSOR]
        )
        cg.add(var.set_mos_temp_sensor(sens))
    if CONF_CELL_TEMP_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_CELL_TEMP_SENSOR]
        )
        cg.add(var.set_cell_temp_sensor(sens))
    if CONF_CURRENT_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_CURRENT_SENSOR]
        )
        cg.add(var.set_current_sensor(sens))
    if CONF_MAX_CELL_VOLTAGE_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_MAX_CELL_VOLTAGE_SENSOR]
        )
        cg.add(
            var.set_max_cell_voltage_sensor(sens))
    if CONF_MIN_CELL_VOLTAGE_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_MIN_CELL_VOLTAGE_SENSOR]
        )
        cg.add(
            var.set_min_cell_voltage_sensor(sens))
    if CONF_CELL_DELTA_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_CELL_DELTA_SENSOR]
        )
        cg.add(
            var.set_cell_delta_sensor(sens))
    if CONF_CELL_COUNT_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_CELL_COUNT_SENSOR]
        )
        cg.add(
            var.set_cell_count_sensor(sens))
    if CONF_TEMP_PROBE1_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_TEMP_PROBE1_SENSOR]
        )
        cg.add(var.set_temp_probe1_sensor(sens))
    if CONF_TEMP_PROBE2_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_TEMP_PROBE2_SENSOR]
        )
        cg.add(var.set_temp_probe2_sensor(sens))
    if CONF_TEMP_PROBE3_SENSOR in config:
        sens = await cg.get_variable(
            config[CONF_TEMP_PROBE3_SENSOR]
        )
        cg.add(var.set_temp_probe3_sensor(sens))
    if CONF_CONNECTED_SENSOR in config:
        sens = await cg.get_variable(
        config[CONF_CONNECTED_SENSOR]
        )
        cg.add(var.set_connected_sensor(sens))
