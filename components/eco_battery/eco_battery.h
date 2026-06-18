#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace eco_battery {

class EcoBattery
    : public Component,
      public ble_client::BLEClientNode {

    public:
        void setup() override;
        void loop() override;
        
        void set_soc_sensor(sensor::Sensor *sensor) {
            this->soc_sensor_ = sensor;
        }
        void set_voltage_sensor(sensor::Sensor *sensor) {
            this->voltage_sensor_ = sensor;
        }
        void set_mos_temp_sensor(sensor::Sensor *sensor) {
            this->mos_temp_sensor_ = sensor;
        }
        void set_cell_temp_sensor(sensor::Sensor *sensor) {
            this->cell_temp_sensor_ = sensor;
        }
        void set_current_sensor(sensor::Sensor *sensor) {
            this->current_sensor_ = sensor;
        }
        void set_max_cell_voltage_sensor(sensor::Sensor *sensor) {
            this->max_cell_voltage_sensor_ = sensor;
        }
        void set_min_cell_voltage_sensor(sensor::Sensor *sensor) {
            this->min_cell_voltage_sensor_ = sensor;
        }
        void set_cell_delta_sensor(sensor::Sensor *sensor) {
            this->cell_delta_sensor_ = sensor;
        }
        void set_cell_count_sensor(sensor::Sensor *sensor) {
            this->cell_count_sensor_ = sensor;
        }
        void set_temp_probe1_sensor(sensor::Sensor *sensor) {
            this->temp_probe1_sensor_ = sensor;
        }
        void set_temp_probe2_sensor(sensor::Sensor *sensor) {
            this->temp_probe2_sensor_ = sensor;
        }
        void set_temp_probe3_sensor(sensor::Sensor *sensor) {
            this->temp_probe3_sensor_ = sensor;
        }
		void set_update_interval(uint32_t interval_ms) {
		    this->update_interval_ms_ = interval_ms;
		}
		void set_connected_sensor(binary_sensor::BinarySensor *s) {
            this->connected_sensor_ = s;
        }

    protected:
        void gattc_event_handler(
            esp_gattc_cb_event_t event,
            esp_gatt_if_t gattc_if,
            esp_ble_gattc_cb_param_t *param
    ) override;
	
	uint32_t update_interval_ms_{600000};
        uint32_t last_poll_{0};
        
        binary_sensor::BinarySensor *connected_sensor_{nullptr};
        
        sensor::Sensor *soc_sensor_{nullptr};
        sensor::Sensor *voltage_sensor_{nullptr};
        sensor::Sensor *mos_temp_sensor_{nullptr};
        sensor::Sensor *cell_temp_sensor_{nullptr};
        sensor::Sensor *current_sensor_{nullptr};
        sensor::Sensor *max_cell_voltage_sensor_{nullptr};
        sensor::Sensor *min_cell_voltage_sensor_{nullptr};
        sensor::Sensor *cell_delta_sensor_{nullptr};
        sensor::Sensor *cell_count_sensor_{nullptr};
        sensor::Sensor *temp_probe1_sensor_{nullptr};
        sensor::Sensor *temp_probe2_sensor_{nullptr};
        sensor::Sensor *temp_probe3_sensor_{nullptr};
};

}  // namespace eco_battery
}  // namespace esphome