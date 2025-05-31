# https://github.com/smartboxchannel/EFEKTA-Open_PM_Monitor
from typing import Final

from zigpy.profiles import zha
from zigpy.quirks import CustomCluster
from zigpy.quirks.v2 import (
    QuirkBuilder,
    ReportingConfig,
    SensorDeviceClass,
    SensorStateClass,
)
from zigpy.quirks.v2.homeassistant.number import NumberDeviceClass
import zigpy.types as t
from zigpy.zcl import ClusterType
from zigpy.zcl.foundation import ZCLAttributeDef
from zigpy.zcl.clusters.measurement import (
    PM25,
)
from zigpy.quirks.v2.homeassistant import (
    CONCENTRATION_MICROGRAMS_PER_CUBIC_METER,
)

EFEKTA = "EfektaLab_for_you"


class PMMeasurement(PM25, CustomCluster):
    class AttributeDefs(PM25.AttributeDefs):
        pm1: Final = ZCLAttributeDef(id=0x0601, type=t.Single, access="r")
        pm10: Final = ZCLAttributeDef(id=0x0602, type=t.Single, access="r")
        aqi_25_index: Final = ZCLAttributeDef(id=0x0604, type=t.Single, access="r")
        indicator_corection: Final = ZCLAttributeDef(id=0x0333, type=t.int8s, access="rw")


(
    QuirkBuilder(EFEKTA, "Open_PM_Monitor")
    .replaces_endpoint(2, device_type=zha.DeviceType.SIMPLE_SENSOR)
    .replaces(PMMeasurement, endpoint_id=2)
    .sensor(
        PMMeasurement.AttributeDefs.pm1.name,
        PMMeasurement.cluster_id,
        endpoint_id=2,
        state_class=SensorStateClass.MEASUREMENT,
        device_class=SensorDeviceClass.PM1,
        reporting_config=ReportingConfig(min_interval=10, max_interval=120, reportable_change=1),
        translation_key="pm1",
        fallback_name="PM1",
        unit=CONCENTRATION_MICROGRAMS_PER_CUBIC_METER,
    )
    .sensor(
        PMMeasurement.AttributeDefs.pm10.name,
        PMMeasurement.cluster_id,
        endpoint_id=2,
        state_class=SensorStateClass.MEASUREMENT,
        device_class=SensorDeviceClass.PM10,
        reporting_config=ReportingConfig(min_interval=10, max_interval=120, reportable_change=1),
        translation_key="pm10",
        fallback_name="PM10",
        unit=CONCENTRATION_MICROGRAMS_PER_CUBIC_METER,
    )
    .sensor(
        PMMeasurement.AttributeDefs.aqi_25_index.name,
        PMMeasurement.cluster_id,
        endpoint_id=2,
        state_class=SensorStateClass.MEASUREMENT,
        device_class=SensorDeviceClass.AQI,
        reporting_config=ReportingConfig(min_interval=10, max_interval=120, reportable_change=1),
        translation_key="aqi_25_index",
        fallback_name="PM 2.5 INDEX",
        unit="PM2.5 Index",
    )
    .number(
        PMMeasurement.AttributeDefs.indicator_corection.name,
        PMMeasurement.cluster_id,
        endpoint_id=2,
        translation_key="indicator_corection",
        fallback_name="Indicator corection",
        unique_id_suffix="indicator_corection",
        min_value=-15,
        max_value=15,
        step=1,
    )
    .add_to_registry()
)