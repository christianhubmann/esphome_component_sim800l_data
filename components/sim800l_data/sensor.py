import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_BATTERY_VOLTAGE,
    CONF_ID,
    CONF_SIGNAL_STRENGTH,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL_MILLIWATT,
    UNIT_PERCENT,
    UNIT_VOLT,
)

from . import Sim800LDataComponent

DEPENDENCIES = ["sim800l_data"]


CONFIG_SCHEMA = {
    cv.GenerateID(): cv.use_id(Sim800LDataComponent),
    cv.Optional(CONF_SIGNAL_STRENGTH): sensor.sensor_schema(
        unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        device_class=DEVICE_CLASS_BATTERY,
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
}


async def to_code(config):
    sim800l_data_component = await cg.get_variable(config[CONF_ID])

    for key in [
        CONF_SIGNAL_STRENGTH,
        CONF_BATTERY_LEVEL,
        CONF_BATTERY_VOLTAGE,
    ]:
        if key not in config:
            continue
        sens = await sensor.new_sensor(config[key])
        cg.add(getattr(sim800l_data_component, f"set_{key}_sensor")(sens))
