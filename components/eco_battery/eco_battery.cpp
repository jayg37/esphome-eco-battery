#include "eco_battery.h"
#include "esphome/core/log.h"
#include <cmath>
#include <vector>

namespace esphome {
namespace eco_battery {

static const char *const TAG = "eco_battery";

static constexpr uint16_t BMS_SERVICE_UUID = 0xFF00;
static constexpr uint16_t BMS_NOTIFY_CHAR_UUID = 0x0011;
static constexpr uint16_t BMS_WRITE_CHAR_UUID = 0x0014;
static constexpr uint16_t BMS_NOTIFY_DESC_UUID = 0x0012;
static constexpr size_t BMS_REGISTER_COUNT = 122;
static constexpr size_t BMS_FRAME_SIZE = 3 + (BMS_REGISTER_COUNT * 2);

static bool notify_enabled = false;
static std::vector<uint8_t> rx_buffer;
static uint32_t last_rx_ms = 0;

static void clear_rx_state() {
  notify_enabled = false;
  rx_buffer.clear();
  last_rx_ms = 0;
}

void EcoBattery::setup() {
  this->last_poll_ = millis() - this->update_interval_ms_;
  this->poll_pending_ = false;
  this->poll_active_ = false;
  this->disconnect_pending_ = false;
  this->response_received_ = false;
}

void EcoBattery::loop() {
  const uint32_t now = millis();

  // A complete response is expected to contain 122 registers:
  // 3 protocol bytes + 244 payload bytes = 247 bytes minimum.
  if (!rx_buffer.empty() && (now - last_rx_ms) > 100) {
    if (rx_buffer.size() < BMS_FRAME_SIZE) {
      ESP_LOGW(TAG, "Incomplete BMS response: received %u bytes, expected at least %u",
               (unsigned) rx_buffer.size(), (unsigned) BMS_FRAME_SIZE);
      rx_buffer.clear();
      if (this->poll_active_) {
        this->poll_active_ = false;
        this->poll_pending_ = false;
        this->disconnect_pending_ = true;
        if (this->parent_ != nullptr && this->parent_->connected()) {
          ESP_LOGW(TAG, "Disconnecting after incomplete BMS response");
          this->parent_->disconnect();
        }
      }
    } else {
      uint16_t regs[BMS_REGISTER_COUNT];

      for (size_t reg = 0; reg < BMS_REGISTER_COUNT; reg++) {
        const size_t offset = 3 + (reg * 2);
        regs[reg] = (static_cast<uint16_t>(rx_buffer[offset]) << 8) |
                    rx_buffer[offset + 1];
      }

      const float soc = static_cast<float>(regs[5]);
      if (this->soc_sensor_ != nullptr)
        this->soc_sensor_->publish_state(soc);

      float voltage = 0.0f;
      float max_cell_voltage = 0.0f;
      float min_cell_voltage = 100.0f;
      for (int i = 33; i <= 48; i++) {
        const float v = static_cast<float>(regs[i]) / 1000.0f;
        voltage += v;
        if (v > max_cell_voltage)
          max_cell_voltage = v;
        if (v < min_cell_voltage)
          min_cell_voltage = v;
      }

      if (this->voltage_sensor_ != nullptr)
        this->voltage_sensor_->publish_state(voltage);

      const float mos_temp = static_cast<float>(regs[22]);
      if (this->mos_temp_sensor_ != nullptr)
        this->mos_temp_sensor_->publish_state(mos_temp);

      const float cell_temp = static_cast<float>(regs[23]);
      if (this->cell_temp_sensor_ != nullptr)
        this->cell_temp_sensor_->publish_state(cell_temp);

      const float current = static_cast<int16_t>(regs[4]) / 10.0f;
      if (this->current_sensor_ != nullptr)
        this->current_sensor_->publish_state(current);

      if (this->max_cell_voltage_sensor_ != nullptr)
        this->max_cell_voltage_sensor_->publish_state(max_cell_voltage);

      if (this->min_cell_voltage_sensor_ != nullptr)
        this->min_cell_voltage_sensor_->publish_state(min_cell_voltage);

      const float cell_delta = max_cell_voltage - min_cell_voltage;
      if (this->cell_delta_sensor_ != nullptr)
        this->cell_delta_sensor_->publish_state(cell_delta);

      const float cell_count = static_cast<float>(regs[2]);
      if (this->cell_count_sensor_ != nullptr)
        this->cell_count_sensor_->publish_state(cell_count);

      if (this->temp_probe1_sensor_ != nullptr)
        this->temp_probe1_sensor_->publish_state(static_cast<float>(regs[23]));
      if (this->temp_probe2_sensor_ != nullptr)
        this->temp_probe2_sensor_->publish_state(static_cast<float>(regs[24]));
      if (this->temp_probe3_sensor_ != nullptr)
        this->temp_probe3_sensor_->publish_state(static_cast<float>(regs[25]));

      ESP_LOGI(TAG,
               "BMS response received: %u bytes, SOC=%.1f%% Voltage=%.2fV Current=%.1fA",
               (unsigned) rx_buffer.size(), soc, voltage, current);

      rx_buffer.clear();
      last_rx_ms = 0;
      this->response_received_ = true;
      this->poll_active_ = false;
      this->poll_pending_ = false;

      if (this->parent_ != nullptr && this->parent_->connected()) {
        this->disconnect_pending_ = true;
        ESP_LOGD(TAG, "Poll complete; disconnecting from BMS");
        this->parent_->disconnect();
      }
    }
  }

  // Start a new poll when due. With auto_connect disabled, connect() is only
  // requested when a poll is actually due.
  if (!this->poll_pending_ && !this->poll_active_ &&
      (now - this->last_poll_) >= this->update_interval_ms_) {
    this->last_poll_ = now;

    if (this->parent_ == nullptr) {
      ESP_LOGE(TAG, "PARENT NULL");
      return;
    }

    if (this->parent_->connected()) {
      ESP_LOGD(TAG, "BMS already connected at poll time; beginning poll");
      this->poll_pending_ = true;
      this->poll_active_ = true;
      this->response_received_ = false;
    } else if (this->parent_->state() == esp32_ble_tracker::ClientState::IDLE) {
      ESP_LOGI(TAG, "Poll due; connecting to Eco Battery BMS");
      this->poll_pending_ = true;
      this->response_received_ = false;
      this->parent_->connect();
    } else {
      ESP_LOGD(TAG, "Poll due but BMS client is in %s; will retry next interval",
               esp32_ble_tracker::client_state_to_string(this->parent_->state()));
      this->last_poll_ = now - this->update_interval_ms_ + 5000;
    }
  }
}

void EcoBattery::send_bms_request_() {
  if (this->parent_ == nullptr || !this->parent_->connected()) {
    ESP_LOGW(TAG, "Cannot poll BMS: BLE client is not established");
    return;
  }

  auto *chr = this->parent_->get_characteristic(BMS_SERVICE_UUID, BMS_WRITE_CHAR_UUID);
  if (chr == nullptr) {
    ESP_LOGE(TAG, "BMS write characteristic 0x%04X not found in service 0x%04X",
             BMS_WRITE_CHAR_UUID, BMS_SERVICE_UUID);
    this->poll_active_ = false;
    this->poll_pending_ = false;
    this->disconnect_pending_ = true;
    this->parent_->disconnect();
    return;
  }

  rx_buffer.clear();
  last_rx_ms = 0;
  this->response_received_ = false;
  this->poll_active_ = true;

  const uint8_t cmd[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x7A, 0xC4, 0x29};
  ESP_LOGD(TAG, "Sending BMS request on characteristic 0x%04X (%u bytes)",
           BMS_WRITE_CHAR_UUID, (unsigned) sizeof(cmd));

  const esp_err_t err = chr->write_value(cmd, sizeof(cmd));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "BMS request write failed: %s", esp_err_to_name(err));
    this->poll_active_ = false;
    this->poll_pending_ = false;
    this->disconnect_pending_ = true;
    this->parent_->disconnect();
  }
}

void EcoBattery::publish_unavailable_() {
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
}

void EcoBattery::gattc_event_handler(esp_gattc_cb_event_t event,
                                      esp_gatt_if_t gattc_if,
                                      esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_CONNECT_EVT:
      ESP_LOGI(TAG, "CONNECTED");
      if (this->connected_sensor_ != nullptr)
        this->connected_sensor_->publish_state(true);
      break;

    case ESP_GATTC_DISCONNECT_EVT:
      ESP_LOGW(TAG, "DISCONNECTED");
      if (this->connected_sensor_ != nullptr)
        this->connected_sensor_->publish_state(false);
      this->publish_unavailable_();
      this->poll_active_ = false;
      this->poll_pending_ = false;
      this->disconnect_pending_ = false;
      clear_rx_state();
      break;

    case ESP_GATTC_SEARCH_CMPL_EVT:
      ESP_LOGI(TAG, "SERVICE SEARCH COMPLETE");

      if (!notify_enabled) {
        notify_enabled = true;
        const esp_err_t err = this->parent_->register_for_notify(0x0011);
        if (err != ESP_OK) {
          ESP_LOGE(TAG, "Failed to register BMS notification characteristic: %s",
                   esp_err_to_name(err));
          this->disconnect_pending_ = true;
          this->parent_->disconnect();
          break;
        }
      }

      // The BLE client action layer changes its node state to ESTABLISHED at
      // SEARCH_CMPL_EVT before this node is dispatched. The service cache is
      // therefore available here.
      if (this->poll_pending_)
        this->send_bms_request_();
      break;

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      const uint16_t notify_on = 1;
      const esp_err_t err = esp_ble_gattc_write_char_descr(
          gattc_if,
          this->parent_->get_conn_id(),
          BMS_NOTIFY_DESC_UUID,
          sizeof(notify_on),
          reinterpret_cast<uint8_t *>(const_cast<uint16_t *>(&notify_on)),
          ESP_GATT_WRITE_TYPE_RSP,
          ESP_GATT_AUTH_REQ_NONE);
      if (err != ESP_OK)
        ESP_LOGW(TAG, "Failed to enable BMS notifications: %s", esp_err_to_name(err));
      break;
    }

    case ESP_GATTC_NOTIFY_EVT: {
      auto &notify = param->notify;
      ESP_LOGD(TAG, "BMS notification: %u bytes", (unsigned) notify.value_len);
      rx_buffer.insert(rx_buffer.end(), notify.value, notify.value + notify.value_len);
      last_rx_ms = millis();
      break;
    }

    case ESP_GATTC_WRITE_CHAR_EVT:
      if (this->poll_active_)
        ESP_LOGD(TAG, "BMS request write acknowledged");
      break;

    default:
      break;
  }
}

}  // namespace eco_battery
}  // namespace esphome
