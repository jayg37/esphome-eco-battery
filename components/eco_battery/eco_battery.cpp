#include "eco_battery.h"
#include "esphome/core/log.h"
#include <vector>

namespace esphome {
namespace eco_battery {

static const char *const TAG = "eco_battery";

static bool notify_enabled = false;

static std::vector<uint8_t> rx_buffer;
static uint32_t last_rx_ms = 0;
//
// Confirmed Register Map
//
// REG[02] Cell Count
// Voltage is calculated from REG[33]-REG[48]
// Current is decoded from REG[4] with /10.0f
// REG[05] State of Charge (%)
//
// REG[22] MOS Temperature (°C)
// REG[23] Temperature Probe 1 (°C)
// REG[24] Temperature Probe 2 (°C)
// REG[25] Temperature Probe 3 (°C)
//
// REG[33] - REG[48] Cell Voltages (mV)
//

void EcoBattery::setup() {

    this->last_poll_ =
        millis() - this->update_interval_ms_;
}
void EcoBattery::loop() {
		
    //
    // Reassemble fragmented FF01 notifications
    //
    if (!rx_buffer.empty() && (millis() - last_rx_ms) > 100) {
                  
        //
        // Decode Modbus registers
        //
		
        uint16_t regs[122];

        for (int reg = 0; reg < 122; reg++) {
            int offset = 3 + (reg * 2);

            regs[reg] =
                ((uint16_t) rx_buffer[offset] << 8) |
                rx_buffer[offset + 1];
        }
        //
        // SOC
        //
        float soc = (float) regs[5];
                
        if (this->soc_sensor_ != nullptr) {
          this->soc_sensor_->publish_state(soc);
        }
      
        //
        // Pack Voltage
        //
        float voltage = 0.0f;

        for (int i = 33; i <= 48; i++) {
            voltage += ((float) regs[i]) / 1000.0f;
        }
                            
        if (this->voltage_sensor_ != nullptr) {
            this->voltage_sensor_->publish_state(voltage);
        }
        //
        //MOS Temperature
        //
        float mos_temp = (float) regs[22];
                
        if (this->mos_temp_sensor_ != nullptr) {
            this->mos_temp_sensor_->publish_state(mos_temp);
        }
        //
        //Cell Temperature
        //
        float cell_temp = (float) regs[23];
               
        if (this->cell_temp_sensor_ != nullptr) {
            this->cell_temp_sensor_->publish_state(cell_temp);
        }
        //
        // Current
        //
        float current =
            ((int16_t) regs[4]) / 10.0f;
                       
        if (this->current_sensor_ != nullptr) {
            this->current_sensor_->publish_state(current);
        }
        //
        //Maximum Cell Voltage
        //
        float max_cell_voltage = 0.0f;

        for (int i = 33; i <= 48; i++) {
            float v = ((float) regs[i]) / 1000.0f;
        
            if (v > max_cell_voltage) {
                max_cell_voltage = v;
            }
        }
              
        if (this->max_cell_voltage_sensor_ != nullptr) {
            this->max_cell_voltage_sensor_->publish_state(
                max_cell_voltage);
        }
        //
        //Minimum Cell Voltage
        //
        float min_cell_voltage = 100.0f;

        for (int i = 33; i <= 48; i++) {
            float v = ((float) regs[i]) / 1000.0f;
        
            if (v < min_cell_voltage) {
                min_cell_voltage = v;
            }
        }
                        
        if (this->min_cell_voltage_sensor_ != nullptr) {
            this->min_cell_voltage_sensor_->publish_state(
                min_cell_voltage);
        }
        //
        //Cell Delta
        //
        float cell_delta =
            max_cell_voltage - min_cell_voltage;
                     
        if (this->cell_delta_sensor_ != nullptr) {
            this->cell_delta_sensor_->publish_state(
                cell_delta);
        }
        //
        // Cell Count
        //
        float cell_count = (float) regs[2];

        if (this->cell_count_sensor_ != nullptr) {
            this->cell_count_sensor_->publish_state(
                cell_count);
        }
        //
        // Temperature Probes
        //
        float temp_probe1 = (float) regs[23];
        float temp_probe2 = (float) regs[24];
        float temp_probe3 = (float) regs[25];

        if (this->temp_probe1_sensor_ != nullptr) {
            this->temp_probe1_sensor_->publish_state(
                temp_probe1);
        }

        if (this->temp_probe2_sensor_ != nullptr) {
            this->temp_probe2_sensor_->publish_state(
                temp_probe2);
        }

        if (this->temp_probe3_sensor_ != nullptr) {
            this->temp_probe3_sensor_->publish_state(
                temp_probe3);
        }        
        
        rx_buffer.clear();
    }

    //
    // Poll battery every 10 minutes
    //
    if ((millis() - this->last_poll_) <
        this->update_interval_ms_)
        return;

    this->last_poll_ = millis();
    
    if (this->parent_ == nullptr) {
        ESP_LOGE(TAG, "PARENT NULL");
        return;
    }

    auto *svc = this->parent_->get_service((uint16_t) 0xFF00);

    if (svc == nullptr)
        return;

    auto *chr = this->parent_->get_characteristic((uint16_t) 0x0014);

    if (chr == nullptr)
        return;

    //
    // Battery request
    //
    uint8_t cmd[] = {
        0x01,
        0x03,
        0x00,
        0x00,
        0x00,
        0x7A,
        0xC4,
        0x29};

    chr->write_value(cmd, sizeof(cmd));  
}

void EcoBattery::gattc_event_handler(
    esp_gattc_cb_event_t event,
    esp_gatt_if_t gattc_if,
    esp_ble_gattc_cb_param_t *param) {

    switch (event) {
        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGI(TAG, "CONNECTED");
            if (this->connected_sensor_ != nullptr) {
                this->connected_sensor_->publish_state(true);
            }
            break;

        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGW(TAG, "DISCONNECTED");
        
            if (this->connected_sensor_ != nullptr) {
                this->connected_sensor_->publish_state(false);
            }
        
            //
            // Mark battery sensors unavailable
            //
            if (this->soc_sensor_ != nullptr)
                this->soc_sensor_->publish_state(NAN);
        
            if (this->voltage_sensor_ != nullptr)
                this->voltage_sensor_->publish_state(NAN);
        
            if (this->current_sensor_ != nullptr)
                this->current_sensor_->publish_state(NAN);
        
            if (this->mos_temp_sensor_ != nullptr)
                this->mos_temp_sensor_->publish_state(NAN);
        
            if (this->cell_temp_sensor_ != nullptr)
                this->cell_temp_sensor_->publish_state(NAN);
        
            if (this->max_cell_voltage_sensor_ != nullptr)
                this->max_cell_voltage_sensor_->publish_state(NAN);
        
            if (this->min_cell_voltage_sensor_ != nullptr)
                this->min_cell_voltage_sensor_->publish_state(NAN);
        
            if (this->cell_delta_sensor_ != nullptr)
                this->cell_delta_sensor_->publish_state(NAN);
        
            if (this->cell_count_sensor_ != nullptr)
                this->cell_count_sensor_->publish_state(NAN);
        
            if (this->temp_probe1_sensor_ != nullptr)
                this->temp_probe1_sensor_->publish_state(NAN);
        
            if (this->temp_probe2_sensor_ != nullptr)
                this->temp_probe2_sensor_->publish_state(NAN);
        
            if (this->temp_probe3_sensor_ != nullptr)
                this->temp_probe3_sensor_->publish_state(NAN);
        
            notify_enabled = false;
            rx_buffer.clear();
            break;

        case ESP_GATTC_SEARCH_CMPL_EVT: {
            ESP_LOGI(TAG, "SERVICE SEARCH COMPLETE");

            if (notify_enabled)
                break;

            notify_enabled = true;

            esp_ble_gattc_register_for_notify(
				gattc_if,
				this->parent_->get_remote_bda(),
				0x0011);

            break;
        }

        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            
            uint16_t notify_on = 1;

            esp_ble_gattc_write_char_descr(
                gattc_if,
                this->parent_->get_conn_id(),
                0x0012,
                sizeof(notify_on),
                (uint8_t *) &notify_on,
                ESP_GATT_WRITE_TYPE_RSP,
                ESP_GATT_AUTH_REQ_NONE);

            break;
        }

        case ESP_GATTC_WRITE_DESCR_EVT:
            break;

        case ESP_GATTC_NOTIFY_EVT: {
            auto &notify = param->notify;

            rx_buffer.insert(
                rx_buffer.end(),
                notify.value,
                notify.value + notify.value_len);

            last_rx_ms = millis();
            break;
        }

        case ESP_GATTC_WRITE_CHAR_EVT:
            break;

        case ESP_GATTC_READ_CHAR_EVT:
            break;

        default:
            break;
    }
}

}  // namespace eco_battery
}  // namespace esphome