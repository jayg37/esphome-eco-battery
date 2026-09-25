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

static constexpr size_t BMS_REGISTER_COUNT = 122;
static constexpr size_t BMS_FRAME_SIZE = 3 + (BMS_REGISTER_COUNT * 2);
static constexpr uint32_t RESPONSE_TIMEOUT_MS = 30000;

static bool notify_enabled = false;
static std::vector<uint8_t> rx_buffer;
static uint32_t last_rx_ms = 0;

static void clear_rx_state() {
  notify_enabled = false;
  rx_buffer.clear();
  last_rx_ms = 0;
}

void EcoBattery::dump_config() {
  ESP_LOGCONFIG(TAG, "Eco Battery:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", (unsigned) this->update_interval_ms_);
  if (this->parent_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  BLE client: NULL");
  } else {
    ESP_LOGCONFIG(TAG, "  BLE client: configured");
    ESP_LOGCONFIG(TAG, "  BLE state: %s",
                  ble_client::espbt::client_state_to_string(this->parent_->state()));
    ESP_LOGCONFIG(TAG, "  BLE connected: %s", this->parent_->connected() ? "YES" : "NO");
  }
}

void EcoBattery::setup() {
  this->last_poll_ = millis() - this->update_interval_ms_;
  this->poll_pending_ = false;
  this->poll_active_ = false;
  this->disconnect_pending_ = false;
  this->poll_started_ms_ = 0;
  this->write_char_handle_ = 0;
  this->notify_char_handle_ = 0;
  this->notify_desc_handle_ = 0;
  this->debug_last_loop_log_ms_ = millis();
  this->debug_loop_count_ = 0;
  this->debug_first_loop_logged_ = false;

  ESP_LOGI(TAG, "SETUP: EcoBattery initialized (interval=%u ms)",
           (unsigned) this->update_interval_ms_);
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "SETUP: BLE parent is NULL");
  } else {
    ESP_LOGI(TAG, "SETUP: BLE parent attached; state=%s connected=%s",
             ble_client::espbt::client_state_to_string(this->parent_->state()),
             this->parent_->connected() ? "YES" : "NO");
  }
}

void EcoBattery::loop() {
  const uint32_t now = millis();
  this->debug_loop_count_++;

  // First-loop proof plus a throttled lifecycle heartbeat. INFO-level is intentional
  // so a field log immediately proves whether EcoBattery::loop() is actually running.
  if (!this->debug_first_loop_logged_) {
    this->debug_first_loop_logged_ = true;
    if (this->parent_ == nullptr) {
      ESP_LOGI(TAG, "LOOP: FIRST PASS count=%u parent=NULL", (unsigned) this->debug_loop_count_);
    } else {
      ESP_LOGI(TAG, "LOOP: FIRST PASS count=%u state=%s connected=%s interval=%u ms",
               (unsigned) this->debug_loop_count_,
               ble_client::espbt::client_state_to_string(this->parent_->state()),
               this->parent_->connected() ? "YES" : "NO",
               (unsigned) this->update_interval_ms_);
    }
  }

  if ((now - this->debug_last_loop_log_ms_) >= 30000) {
    this->debug_last_loop_log_ms_ = now;
    if (this->parent_ == nullptr) {
      ESP_LOGI(TAG, "LOOP: alive count=%u parent=NULL pending=%s active=%s",
               (unsigned) this->debug_loop_count_,
               this->poll_pending_ ? "YES" : "NO",
               this->poll_active_ ? "YES" : "NO");
    } else {
      ESP_LOGI(TAG, "LOOP: alive count=%u state=%s connected=%s pending=%s active=%s rx=%u",
               (unsigned) this->debug_loop_count_,
               ble_client::espbt::client_state_to_string(this->parent_->state()),
               this->parent_->connected() ? "YES" : "NO",
               this->poll_pending_ ? "YES" : "NO",
               this->poll_active_ ? "YES" : "NO",
               (unsigned) rx_buffer.size());
    }
  }

  if (this->poll_active_ && this->poll_started_ms_ != 0 &&
      (now - this->poll_started_ms_) > RESPONSE_TIMEOUT_MS) {
    ESP_LOGW(TAG, "BMS response timeout after %u ms; disconnecting",
             (unsigned) (now - this->poll_started_ms_));
    this->poll_active_ = false;
    this->poll_pending_ = false;
    this->disconnect_pending_ = true;
    rx_buffer.clear();
    if (this->parent_ != nullptr && this->parent_->connected())
      this->parent_->disconnect();
    return;
  }

  if (!rx_buffer.empty() && (now - last_rx_ms) > 100) {
    if (rx_buffer.size() < BMS_FRAME_SIZE) {
      ESP_LOGW(TAG, "Incomplete BMS response: received %u bytes, expected at least %u",
               (unsigned) rx_buffer.size(), (unsigned) BMS_FRAME_SIZE);
      rx_buffer.clear();
      this->poll_active_ = false;
      this->poll_pending_ = false;
      this->disconnect_pending_ = true;
      if (this->parent_ != nullptr && this->parent_->connected()) {
        ESP_LOGW(TAG, "Disconnecting after incomplete BMS response");
        this->parent_->disconnect();
      }
    } else {
      uint16_t regs[BMS_REGISTER_COUNT];
      for (size_t reg = 0; reg < BMS_REGISTER_COUNT; reg++) {
        const size_t offset = 3 + (reg * 2);
        regs[reg] = (static_cast<uint16_t>(rx_buffer[offset]) << 8) |
                    rx_buffer[offset + 1];
      }

      const float soc = static_cast<float>(regs[5]);
      float voltage = 0.0f;
      float max_cell_voltage = 0.0f;
      float min_cell_voltage = 100.0f;

      for (int i = 33; i <= 48; i++) {
        const float v = static_cast<float>(regs[i]) / 1000.0f;
        voltage += v;
        if (v > max_cell_voltage) max_cell_voltage = v;
        if (v < min_cell_voltage) min_cell_voltage = v;
      }

      const float mos_temp = static_cast<float>(regs[22]);
      const float cell_temp = static_cast<float>(regs[23]);
      const float current = static_cast<int16_t>(regs[4]) / 10.0f;
      const float cell_delta = max_cell_voltage - min_cell_voltage;
      const float cell_count = static_cast<float>(regs[2]);

      if (this->soc_sensor_ != nullptr) this->soc_sensor_->publish_state(soc);
      if (this->voltage_sensor_ != nullptr) this->voltage_sensor_->publish_state(voltage);
      if (this->mos_temp_sensor_ != nullptr) this->mos_temp_sensor_->publish_state(mos_temp);
      if (this->cell_temp_sensor_ != nullptr) this->cell_temp_sensor_->publish_state(cell_temp);
      if (this->current_sensor_ != nullptr) this->current_sensor_->publish_state(current);
      if (this->max_cell_voltage_sensor_ != nullptr)
        this->max_cell_voltage_sensor_->publish_state(max_cell_voltage);
      if (this->min_cell_voltage_sensor_ != nullptr)
        this->min_cell_voltage_sensor_->publish_state(min_cell_voltage);
      if (this->cell_delta_sensor_ != nullptr) this->cell_delta_sensor_->publish_state(cell_delta);
      if (this->cell_count_sensor_ != nullptr) this->cell_count_sensor_->publish_state(cell_count);
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
      this->poll_active_ = false;
      this->poll_pending_ = false;
      this->poll_started_ms_ = 0;

      if (this->parent_ != nullptr && this->parent_->connected()) {
        this->disconnect_pending_ = true;
        ESP_LOGI(TAG, "Poll complete; disconnecting from BMS");
        this->parent_->disconnect();
      }
    }
  }

  if (!this->poll_pending_ && !this->poll_active_ &&
      (now - this->last_poll_) >= this->update_interval_ms_) {
    this->last_poll_ = now;

    if (this->parent_ == nullptr) {
      ESP_LOGE(TAG, "POLL: parent BLE client is NULL");
      return;
    }

    if (this->parent_->connected()) {
      ESP_LOGW(TAG, "POLL: BLE client already connected; using existing connection");
      this->poll_pending_ = true;
      return;
    }

    if (this->parent_->state() == ble_client::espbt::ClientState::IDLE) {
      ESP_LOGI(TAG, "POLL: due; state=%s connected=%s -> calling connect()",
               ble_client::espbt::client_state_to_string(this->parent_->state()),
               this->parent_->connected() ? "YES" : "NO");
      this->poll_pending_ = true;
      this->parent_->connect();
      ESP_LOGD(TAG, "POLL: connect() returned; state=%s connected=%s",
               ble_client::espbt::client_state_to_string(this->parent_->state()),
               this->parent_->connected() ? "YES" : "NO");
    } else {
      ESP_LOGW(TAG, "POLL: due but BLE client is in %s; retrying next interval",
               ble_client::espbt::client_state_to_string(this->parent_->state()));
      this->last_poll_ = now;
    }
  }
}

void EcoBattery::send_bms_request_() {
  if (this->parent_ == nullptr || !this->parent_->connected()) {
    ESP_LOGW(TAG, "Cannot poll BMS: BLE client is not established");
    return;
  }

  if (this->write_char_handle_ == 0) {
    ESP_LOGE(TAG, "BMS write characteristic handle is not available");
    this->poll_active_ = false;
    this->poll_pending_ = false;
    this->disconnect_pending_ = true;
    this->parent_->disconnect();
    return;
  }

  rx_buffer.clear();
  last_rx_ms = 0;
  this->poll_started_ms_ = millis();
  this->poll_active_ = true;

  const uint8_t cmd[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x7A, 0xC4, 0x29};

  ESP_LOGD(TAG, "Sending BMS request: 01 03 00 00 00 7A C4 29");

  const esp_err_t err = esp_ble_gattc_write_char(
      this->parent_->get_gattc_if(),
      this->parent_->get_conn_id(),
      this->write_char_handle_,
      sizeof(cmd),
      const_cast<uint8_t *>(cmd),
      ESP_GATT_WRITE_TYPE_RSP,
      ESP_GATT_AUTH_REQ_NONE);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "BMS request write failed: %s", esp_err_to_name(err));
    this->poll_active_ = false;
    this->poll_pending_ = false;
    this->disconnect_pending_ = true;
    this->parent_->disconnect();
  }
}

void EcoBattery::publish_unavailable_() {
  if (this->soc_sensor_ != nullptr) this->soc_sensor_->publish_state(NAN);
  if (this->voltage_sensor_ != nullptr) this->voltage_sensor_->publish_state(NAN);
  if (this->current_sensor_ != nullptr) this->current_sensor_->publish_state(NAN);
  if (this->mos_temp_sensor_ != nullptr) this->mos_temp_sensor_->publish_state(NAN);
  if (this->cell_temp_sensor_ != nullptr) this->cell_temp_sensor_->publish_state(NAN);
  if (this->max_cell_voltage_sensor_ != nullptr) this->max_cell_voltage_sensor_->publish_state(NAN);
  if (this->min_cell_voltage_sensor_ != nullptr) this->min_cell_voltage_sensor_->publish_state(NAN);
  if (this->cell_delta_sensor_ != nullptr) this->cell_delta_sensor_->publish_state(NAN);
  if (this->cell_count_sensor_ != nullptr) this->cell_count_sensor_->publish_state(NAN);
  if (this->temp_probe1_sensor_ != nullptr) this->temp_probe1_sensor_->publish_state(NAN);
  if (this->temp_probe2_sensor_ != nullptr) this->temp_probe2_sensor_->publish_state(NAN);
  if (this->temp_probe3_sensor_ != nullptr) this->temp_probe3_sensor_->publish_state(NAN);
}

void EcoBattery::gattc_event_handler(
    esp_gattc_cb_event_t event,
    esp_gatt_if_t gattc_if,
    esp_ble_gattc_cb_param_t *param) {

  switch (event) {
    case ESP_GATTC_CONNECT_EVT:
      ESP_LOGI(TAG, "GATT EVENT: CONNECTED (if=%d conn_id=%d)",
               gattc_if, param->connect.conn_id);
      if (this->connected_sensor_ != nullptr)
        this->connected_sensor_->publish_state(true);
      break;

    case ESP_GATTC_DISCONNECT_EVT:
      ESP_LOGW(TAG, "GATT EVENT: DISCONNECTED (if=%d reason=%d)",
               gattc_if, param->disconnect.reason);
      if (this->connected_sensor_ != nullptr)
        this->connected_sensor_->publish_state(false);
      this->publish_unavailable_();
      this->poll_active_ = false;
      this->poll_pending_ = false;
      this->disconnect_pending_ = false;
      this->poll_started_ms_ = 0;
      this->write_char_handle_ = 0;
      this->notify_char_handle_ = 0;
      this->notify_desc_handle_ = 0;
      clear_rx_state();
      break;

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGI(TAG, "GATT EVENT: SERVICE SEARCH COMPLETE");

      ESP_LOGD(TAG, "DISCOVERY: looking for service 0x%04X", BMS_SERVICE_UUID);
      auto *service = this->parent_->get_service(BMS_SERVICE_UUID);
      if (service == nullptr) {
        ESP_LOGE(TAG, "DISCOVERY: service lookup failed; BLE state=%s",
                 ble_client::espbt::client_state_to_string(this->parent_->state()));
        ESP_LOGE(TAG, "BMS service 0x%04X not found", BMS_SERVICE_UUID);
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      // Use handle-based characteristic discovery across the discovered GATT cache.
      // This matches the working main-branch implementation and avoids relying on
      // the service/characteristic UUID cache lookup during SEARCH_CMPL_EVT.
      ESP_LOGD(TAG, "DISCOVERY: service found; looking for notify=0x%04X write=0x%04X",
               BMS_NOTIFY_CHAR_UUID, BMS_WRITE_CHAR_UUID);
      auto *notify_char = this->parent_->get_characteristic(BMS_NOTIFY_CHAR_UUID);
      auto *write_char = this->parent_->get_characteristic(BMS_WRITE_CHAR_UUID);

      ESP_LOGD(TAG, "DISCOVERY: notify=%s write=%s",
               notify_char != nullptr ? "FOUND" : "MISSING",
               write_char != nullptr ? "FOUND" : "MISSING");
      if (notify_char == nullptr || write_char == nullptr) {
        ESP_LOGE(TAG, "BMS characteristics not found (notify=0x%04X write=0x%04X)",
                 BMS_NOTIFY_CHAR_UUID, BMS_WRITE_CHAR_UUID);
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      auto *notify_desc = this->parent_->get_config_descriptor(notify_char->handle);

      ESP_LOGD(TAG, "DISCOVERY: notification descriptor lookup result=%s",
               notify_desc != nullptr ? "FOUND" : "MISSING");
      if (notify_desc == nullptr) {
        ESP_LOGE(TAG, "BMS notification configuration descriptor not found for notify handle 0x%04X",
                 notify_char->handle);
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      this->notify_char_handle_ = notify_char->handle;
      this->write_char_handle_ = write_char->handle;
      this->notify_desc_handle_ = notify_desc->handle;

      ESP_LOGI(TAG, "DISCOVERY: handles notify=0x%04X write=0x%04X desc=0x%04X",
               this->notify_char_handle_, this->write_char_handle_, this->notify_desc_handle_);

      ESP_LOGD(TAG, "DISCOVERY: registering for notifications on handle 0x%04X",
               this->notify_char_handle_);
      const esp_err_t err = this->parent_->register_for_notify(this->notify_char_handle_);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register for BMS notifications: %s",
                 esp_err_to_name(err));
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      notify_enabled = true;
      ESP_LOGD(TAG, "DISCOVERY: register_for_notify() returned %s", esp_err_to_name(err));
      break;
    }

    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
      ESP_LOGD(TAG, "GATT EVENT: REG_FOR_NOTIFY status=%d handle=0x%04X",
               param->reg_for_notify.status, param->reg_for_notify.handle);
      if (param->reg_for_notify.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "BMS notification registration failed: status=%d",
                 param->reg_for_notify.status);
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      ESP_LOGI(TAG, "GATT EVENT: notification registration complete");

      if (this->notify_desc_handle_ == 0) {
        ESP_LOGE(TAG, "BMS notification descriptor handle is unavailable");
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      {
        const uint16_t notify_on = 1;
        const esp_err_t err = esp_ble_gattc_write_char_descr(
            gattc_if,
            this->parent_->get_conn_id(),
            this->notify_desc_handle_,
            sizeof(notify_on),
            reinterpret_cast<uint8_t *>(const_cast<uint16_t *>(&notify_on)),
            ESP_GATT_WRITE_TYPE_RSP,
            ESP_GATT_AUTH_REQ_NONE);

        if (err != ESP_OK) {
          ESP_LOGE(TAG, "Failed to enable BMS notifications: %s",
                   esp_err_to_name(err));
          this->poll_pending_ = false;
          this->parent_->disconnect();
        }
      }
      break;

    case ESP_GATTC_WRITE_DESCR_EVT:
      ESP_LOGD(TAG, "GATT EVENT: WRITE_DESCR status=%d handle=0x%04X",
               param->write.status, param->write.handle);
      if (param->write.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "BMS notification descriptor write failed: status=%d",
                 param->write.status);
        this->poll_pending_ = false;
        this->parent_->disconnect();
        break;
      }

      ESP_LOGI(TAG, "GATT EVENT: BMS notifications enabled; sending request");
      this->node_state = ble_client::espbt::ClientState::ESTABLISHED;
      this->send_bms_request_();
      break;

    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGD(TAG, "GATT EVENT: NOTIFY handle=0x%04X len=%u",
               param->notify.handle, (unsigned) param->notify.value_len);
      auto &notify = param->notify;
      if (notify.handle != this->notify_char_handle_) {
        ESP_LOGD(TAG, "Ignoring notification from unexpected handle 0x%04X",
                 notify.handle);
        break;
      }
      if (notify.value_len == 0) {
        ESP_LOGW(TAG, "Ignoring empty BMS notification");
        break;
      }

      ESP_LOGD(TAG, "BMS notification: %u bytes", (unsigned) notify.value_len);
      rx_buffer.insert(rx_buffer.end(), notify.value, notify.value + notify.value_len);
      last_rx_ms = millis();
      break;
    }

    case ESP_GATTC_WRITE_CHAR_EVT:
      ESP_LOGD(TAG, "GATT EVENT: WRITE_CHAR status=%d handle=0x%04X",
               param->write.status, param->write.handle);
      if (this->poll_active_)
        ESP_LOGD(TAG, "BMS request write acknowledged");
      break;

    default:
      ESP_LOGV(TAG, "GATT EVENT: event=%d", event);
      break;
  }
}

}  // namespace eco_battery
}  // namespace esphome
