#include "sim800l_data.h"

namespace esphome {
namespace sim800l_data {

void Sim800LDataComponent::setup() {
  // wait a bit before initialization starts
  this->state_ = State::INIT;
  this->wait_.start(1000);
}

void Sim800LDataComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Sim800LData:");
  ESP_LOGCONFIG(TAG, "  SIM PIN: %s", this->pin_.c_str());
  ESP_LOGCONFIG(TAG, "  APN: %s", this->apn_.c_str());
  ESP_LOGCONFIG(TAG, "  APN User: %s", this->apn_user_.c_str());
  ESP_LOGCONFIG(TAG, "  APN Password: %s", this->apn_password_.c_str());
  ESP_LOGCONFIG(TAG, "  Idle Sleep: %s", YESNO(this->idle_sleep_));
#ifdef USE_SENSOR
  LOG_SENSOR("  ", "Signal Strength", this->signal_strength_sensor_);
  LOG_SENSOR("  ", "Battery Level", this->battery_level_sensor_);
  LOG_SENSOR("  ", "Battery Voltage", this->battery_voltage_sensor_);
#endif
}

void Sim800LDataComponent::update() {
  // do nothing if we are waiting, or a command is pending
  if (this->wait_.is_waiting() || this->command_state_.is_pending) {
    return;
  }

  // if we are idle, restart all checks
  if (this->state_ == State::IDLE) {
    this->state_ = State::INIT;
  }
}

void Sim800LDataComponent::loop() {
  if (!this->handle_response_()) {
    return;
  }

  // do nothing if we are waiting
  if (this->wait_.is_waiting()) {
    return;
  }

  // Send command. While command execution is pending, we will not reach this
  // point again; only after a command succeeded or failed.
  switch (this->state_) {
    case State::INIT:
    INIT:
      this->await_ok_("", State::DISABLE_ECHO);
      if (idle_sleep_active_) {
        this->wait_.start(AT_SLEEP_WAIT);
      }
      break;

    case State::DISABLE_ECHO: {
      const State next_state = idle_sleep_active_ ? State::DISABLE_SLEEP : State::CHECK_BATTERY;
      this->await_ok_("E0", next_state);
    } break;

    case State::DISABLE_SLEEP:
      this->await_ok_("+CSCLK=0", State::CHECK_BATTERY);
      idle_sleep_active_ = false;
      break;

    case State::CHECK_BATTERY:
      this->await_response_("+CBC", State::CHECK_BATTERY_RESPONSE);
      break;

    case State::CHECK_BATTERY_RESPONSE: {
      uint8_t bcs, percent;
      uint16_t voltage;
      get_response_param(this->command_state_.response, bcs, percent, voltage);
      ESP_LOGI(TAG, "Battery: %d%%, %dmV", percent, voltage);
      this->state_ = State::CHECK_PIN;
#ifdef USE_SENSOR
      if (this->battery_level_sensor_ != nullptr) {
        this->battery_level_sensor_->publish_state(percent);
      }
      if (this->battery_voltage_sensor_ != nullptr) {
        this->battery_voltage_sensor_->publish_state(voltage / 1000.0f);
      }
#endif
    } break;

    case State::CHECK_PIN:
      this->await_response_("+CPIN?", State::CHECK_PIN_RESPONSE);
      break;

    case State::CHECK_PIN_RESPONSE: {
      std::string code;
      if (!this->command_state_.urc.empty()) {
        get_response_param(this->command_state_.urc, code);
      } else {
        get_response_param(this->command_state_.response, code);
      }

      if (code == READY) {
        this->state_ = State::SET_CONTYPE_GRPS;
        goto SET_CONTYPE_GRPS;
      } else if (code == SIM_PIN) {
        if (this->pin_.empty()) {
          ESP_LOGE(TAG, "SIM is locked, but no SIM PIN configured.");
          this->state_ = State::INIT;
          this->wait_.start(FUTILE_WAIT);
        } else {
          // Enter PIN. If it's correct, module will answer with OK and then URC "READY".
          // If it's wrong, module will simply answer with ERROR. In that case, do not try
          // again because after 3 tries the SIM will become locked with PUK.
          const std::string cmd = str_concat("+CPIN=\"", this->pin_, "\"");
          this->await_urc_(cmd, State::CHECK_PIN_RESPONSE, State::WRONG_PIN);
        }
      } else if (code == SIM_PUK) {
        ESP_LOGE(TAG, "SIM is locked with PUK. Use another device to unlock it.");
        this->state_ = State::INIT;
        this->wait_.start(FUTILE_WAIT);
      } else {
        ESP_LOGE(TAG, "SIM is locked with status: %s", code.c_str());
        this->state_ = State::INIT;
        this->wait_.start(FUTILE_WAIT);
      }
    } break;

    case State::WRONG_PIN:
      ESP_LOGE(TAG, "Wrong PIN. Component will stop.");
      this->state_ = State::FATAL;
      break;

    case State::SET_CONTYPE_GRPS:
    SET_CONTYPE_GRPS:
      this->await_ok_("+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", State::SET_APN);
      break;

    case State::SET_APN:
      if (!this->apn_.empty()) {
        const std::string cmd = str_concat("+SAPBR=3,1,\"APN\",\"", this->apn_, "\"");
        this->await_ok_(cmd, State::SET_APN_USER);
        break;
      }
      this->state_ = State::SET_APN_USER;

    case State::SET_APN_USER:
      if (!this->apn_user_.empty()) {
        const std::string cmd = str_concat("+SAPBR=3,1,\"USER\",\"", this->apn_user_, "\"");
        this->await_ok_(cmd, State::SET_APN_PWD);
        break;
      }
      this->state_ = State::SET_APN_PWD;

    case State::SET_APN_PWD:
      if (!this->apn_password_.empty()) {
        const std::string cmd = str_concat("+SAPBR=3,1,\"PWD\",\"", this->apn_password_, "\"");
        this->await_ok_(cmd, State::CHECK_REGISTRATION);
        break;
      }
      this->state_ = State::CHECK_REGISTRATION;

    case State::CHECK_REGISTRATION:
      this->await_response_("+CREG?", State::CHECK_REGISTRATION_RESPONSE);
      break;

    case State::CHECK_REGISTRATION_RESPONSE: {
      uint8_t n, stat;
      get_response_param(this->command_state_.response, n, stat);
      if (stat != 1 && stat != 5) {
        ESP_LOGE(TAG, "Not registered to network.");
        this->state_ = State::INIT;
        this->wait_.start(NOT_REGISTERED_WAIT);
      } else {
        this->state_ = State::CHECK_SIGNAL_QUALITY;
      }
    } break;

    case State::CHECK_SIGNAL_QUALITY:
      this->await_response_("+CSQ", State::CHECK_SIGNAL_QUALITY_RESPONSE);
      break;

    case State::CHECK_SIGNAL_QUALITY_RESPONSE: {
      uint8_t rssi;
      get_response_param(this->command_state_.response, rssi);
      const int8_t dbm = get_rssi_dbm(rssi);
      ESP_LOGI(TAG, "RSSI: %d dBm", dbm);
      this->state_ = State::IDLE;
#ifdef USE_SENSOR
      if (this->signal_strength_sensor_ != nullptr) {
        this->signal_strength_sensor_->publish_state(dbm);
      }
#endif
    } break;

    case State::IDLE:
      // If there is a pending http request, start it now
      if (this->http_state_.state == HttpState::QUEUED) {
        // If idle_sleep is active, INIT first. This will disable idle_sleep
        // and we will reach this point again after INIT.
        if (idle_sleep_active_) {
          goto INIT;
        } else {
          goto HTTP_INIT;
        }
      }
      // If nothing to do, start idle sleep if configured
      else if (this->idle_sleep_ && !idle_sleep_active_) {
        goto ENABLE_SLEEP;
      }
      break;

    case State::FATAL:
      ESP_LOGE(TAG, "Fatal error.");
      this->wait_.start(FUTILE_WAIT);
      break;

    case State::ENABLE_SLEEP:
    ENABLE_SLEEP:
      // Enable auto sleep. To wake the module, AT must be sent.
      this->await_ok_("+CSCLK=2", State::IDLE);
      this->idle_sleep_active_ = true;
      break;

    case State::HTTP_INIT:
    HTTP_INIT:
      // Ignore failure. Assume that HTTP is already initialized.
      // If it's not, the next command will fail anyway.
      this->http_state_.state = HttpState::PENDING;
      this->await_ok_("+HTTPINIT", State::HTTP_SET_SSL, State::HTTP_SET_SSL);
      break;

    case State::HTTP_SET_SSL:
      this->await_ok_("+HTTPSSL=" + to_string(this->http_state_.ssl), State::HTTP_SET_BEARER, State::HTTP_FAILED);
      break;

    case State::HTTP_SET_BEARER:
      this->await_ok_("+HTTPPARA=\"CID\",1", State::HTTP_OPEN_BEARER, State::HTTP_FAILED);
      break;

    case State::HTTP_OPEN_BEARER:
      this->await_ok_("+SAPBR=1,1", State::HTTP_SET_URL, State::HTTP_FAILED, BEARER_OPEN_TIMEOUT);
      break;

    case State::HTTP_SET_URL: {
      const std::string cmd = str_concat("+HTTPPARA=\"URL\",\"", this->http_state_.url, "\"");
      this->await_ok_(cmd, State::HTTP_ACTION, State::HTTP_FAILED);
    } break;

    case State::HTTP_ACTION:
      this->await_urc_("+HTTPACTION=0", State::HTTP_ACTION_RESPONSE, State::HTTP_FAILED);
      break;

    case State::HTTP_ACTION_RESPONSE: {
      uint8_t method;
      uint16_t status_code;
      uint32_t length;
      get_response_param(this->command_state_.urc, method, status_code, length);

      this->http_state_.status_code = status_code;

      // Status codes in the 600 range are errors of the module
      if (status_code >= 600 && status_code <= 699) {
        goto HTTP_FAILED;
      }

      ESP_LOGI(TAG, "HTTP request succeeded: %d %s", status_code, this->http_state_.url.c_str());

      // If length is 0, we don't need to send a HTTPREAD command and
      // can directly trigger http request done
      if (length == 0) {
        ESP_LOGV(TAG, "Response body is empty, skip +HTTPREAD");
        goto HTTP_READ_RESPONSE;
      }

      if (length > MAX_HTTP_RESPONSE_SIZE) {
        ESP_LOGW(TAG, "Response body is too big, truncating to %d bytes", MAX_HTTP_RESPONSE_SIZE);
        length = MAX_HTTP_RESPONSE_SIZE;
      }
      this->await_data_("+HTTPREAD", length, State::HTTP_READ_RESPONSE, State::HTTP_FAILED);
    } break;

    case State::HTTP_READ_RESPONSE: {
    HTTP_READ_RESPONSE:
      this->http_state_.response_data = std::move(this->command_state_.data);
      this->http_request_done_callback_.call(this->http_state_.status_code, this->http_state_.response_data);
      this->http_state_.reset();
      this->state_ = State::HTTP_TERM;
    } break;

    case State::HTTP_FAILED:
    HTTP_FAILED:
      ESP_LOGE(TAG, "HTTP request failed: %s", this->http_state_.url.c_str());
      this->state_ = State::HTTP_TERM;
      this->http_request_failed_callback_.call();
      this->http_state_.reset();
      break;

    case State::HTTP_TERM:
      this->await_ok_("+HTTPTERM", State::HTTP_CLOSE_BEARER, State::HTTP_CLOSE_BEARER);
      break;

    case State::HTTP_CLOSE_BEARER:
      this->await_ok_("+SAPBR=0,1", State::IDLE, State::INIT, BEARER_CLOSE_TIMEOUT);
      break;
  }
}

bool Sim800LDataComponent::read_line_() {
  while (this->available()) {
    if (this->read_buffer_.size() >= MAX_READ_BUFFER_SIZE) {
      ESP_LOGE(TAG, "Read buffer full, purging");
      this->read_buffer_.clear();
      this->read_buffer_.shrink_to_fit();
      continue;
    }

    uint8_t byte;
    this->read_byte(&byte);
    ESP_LOGVV(TAG, "--> %02X", byte);

    // Ignore \r and \0
    if (byte == CR || byte == 0) {
      continue;
    }

    if (byte == LF) {
      // ignore empty lines
      if (this->read_buffer_.empty()) {
        continue;
      }
      ESP_LOGV(TAG, "--> %s", this->read_buffer_.c_str());
      return true;
    }

    this->read_buffer_ += static_cast<char>(byte);
  }
  return false;
}

bool Sim800LDataComponent::read_bytes_(const uint16_t length) {
  const uint16_t to_read = length > MAX_READ_BUFFER_SIZE ? MAX_READ_BUFFER_SIZE : length;

  while (this->available() && this->read_buffer_.size() < to_read) {
    uint8_t byte;
    this->read_byte(&byte);
    ESP_LOGVV(TAG, "--> %02X", byte);

    // Replace \0 with space because it would terminate the string,
    // but keep the data length same.
    if (byte == 0) {
      byte = ' ';
    }

    this->read_buffer_ += static_cast<char>(byte);
  }
  return !this->read_buffer_.empty();
}

bool Sim800LDataComponent::handle_response_() {
  // This function returns false, unless a pending command is completed
  // and no more next line is available, so that all input is read until
  // the state machine in loop() advances.

  CommandState &cmd = this->command_state_;

  // If a command is pending that is in the process of receiving variable data,
  // we call read_data_ so that line breaks are not filtered out.
  if (cmd.is_pending && cmd.data_required > 0 && cmd.response_received() && !cmd.data_complete()) {
    const uint32_t data_left = cmd.data_required - cmd.data.size();
    const bool data_read = this->read_bytes_(data_left);

    if (data_read) {
      cmd.data += this->read_buffer_;
      this->read_buffer_.clear();
      this->read_buffer_.shrink_to_fit();
    } else if (cmd.timed_out()) {
      ESP_LOGE(TAG, "Command \"AT%s\" timed out after %d ms", cmd.command.c_str(), cmd.runtime());
      cmd.is_pending = false;
      this->state_ = cmd.error_state;
      this->wait_.start(ERROR_WAIT);
    }
    return false;
  }

  const bool read = this->read_line_();
  if (read) {
    if (!cmd.is_pending) {
      ESP_LOGV(TAG, "Ignoring: \"%s\"", this->read_buffer_.c_str());
      this->read_buffer_.clear();
      return false;
    }

    // TODO: +CME ERROR

    if (this->read_buffer_ == OK) {
      if (cmd.ok_received) {
        ESP_LOGV(TAG, "Ignoring: \"%s\"", this->read_buffer_.c_str());
        this->read_buffer_.clear();
        return false;
      }
      this->read_buffer_.clear();
      cmd.ok_received = true;

      if (cmd.response_required && !cmd.response_received()) {
        ESP_LOGE(TAG, "Command \"AT%s\" failed: missing response", cmd.command.c_str());
        cmd.is_pending = false;
        this->state_ = cmd.error_state;
        this->wait_.start(ERROR_WAIT);
        return false;
      }

      if (cmd.data_required > 0 && !cmd.data_complete()) {
        ESP_LOGE(TAG, "Command \"AT%s\" failed: missing data", cmd.command.c_str());
        cmd.is_pending = false;
        this->state_ = cmd.error_state;
        this->wait_.start(ERROR_WAIT);
        return false;
      }

      if (cmd.urc_required) {
        ESP_LOGI(TAG, "Command \"AT%s\" received OK, waiting for URC", cmd.command.c_str());
        return false;
      }

      cmd.is_pending = false;
      this->state_ = cmd.success_state;
      ESP_LOGI(TAG, "Command \"AT%s\" succeeded after %d ms", cmd.command.c_str(), cmd.runtime());
      return true;
    }

    if (this->read_buffer_ == ERROR) {
      this->read_buffer_.clear();
      ESP_LOGE(TAG, "Command \"AT%s\" failed after %d ms", cmd.command.c_str(), cmd.runtime());
      cmd.is_pending = false;
      this->state_ = cmd.error_state;
      this->wait_.start(ERROR_WAIT);
      return false;
    }

    if (cmd.response_required && !cmd.response_received() && is_response_or_urc(cmd.command, this->read_buffer_)) {
      cmd.response = std::move(this->read_buffer_);
      this->read_buffer_.clear();

      if (cmd.data_required > 0) {
        ESP_LOGI(TAG, "Command \"AT%s\" received response, waiting for data", cmd.command.c_str());
        return false;
      }

      ESP_LOGI(TAG, "Command \"AT%s\" received response, waiting for OK", cmd.command.c_str());
      return false;
    }

    if (cmd.urc_required && !cmd.urc_received() && cmd.ok_received &&
        is_response_or_urc(cmd.command, this->read_buffer_)) {
      cmd.urc = std::move(this->read_buffer_);
      this->read_buffer_.clear();
      cmd.is_pending = false;
      this->state_ = cmd.success_state;
      ESP_LOGI(TAG, "Command AT%s succeeded after %d ms", cmd.command.c_str(), cmd.runtime());
      return true;
    }

    ESP_LOGV(TAG, "Ignoring: \"%s\"", this->read_buffer_.c_str());
    this->read_buffer_.clear();
    return false;
  }

  if (cmd.is_pending) {
    if (cmd.timed_out()) {
      cmd.is_pending = false;
      this->state_ = cmd.error_state;
      ESP_LOGE(TAG, "Command \"AT%s\" timed out after %d ms", cmd.command.c_str(), cmd.runtime());
      this->wait_.start(ERROR_WAIT);
    }
    return false;
  }
  return true;
}

void Sim800LDataComponent::write_(const std::string &s) {
  ESP_LOGV(TAG, "<-- %s", s.c_str());
  this->write_str(s.c_str());
}

void Sim800LDataComponent::write_line_(const std::string &s) {
  ESP_LOGV(TAG, "<-- %s", s.c_str());
  this->write_str(s.c_str());
  this->write_byte(CR);
  this->write_byte(LF);
}

void Sim800LDataComponent::await_ok_(const std::string &command, State success_state, State error_state,
                                     uint32_t timeout) {
  this->command_state_.reset(command, success_state, error_state, timeout);
  this->write_(AT);
  this->write_line_(command);
  this->command_state_.started();
}

void Sim800LDataComponent::await_response_(const std::string &command, State success_state, State error_state,
                                           uint32_t timeout) {
  this->command_state_.reset(command, success_state, error_state, timeout);
  this->command_state_.response_required = true;
  this->write_(AT);
  this->write_line_(command);
  this->command_state_.started();
}

void Sim800LDataComponent::await_urc_(const std::string &command, State success_state, State error_state,
                                      uint32_t timeout, uint32_t urc_timeout) {
  this->command_state_.reset(command, success_state, error_state, timeout);
  this->command_state_.urc_timeout = urc_timeout;
  this->command_state_.urc_required = true;
  this->write_(AT);
  this->write_line_(command);
  this->command_state_.started();
}

void Sim800LDataComponent::await_data_(const std::string &command, uint32_t data_length, State success_state,
                                       State error_state, uint32_t timeout) {
  this->command_state_.reset(command, success_state, error_state, timeout);
  this->command_state_.response_required = true;
  this->command_state_.data_required = data_length;
  this->write_(AT);
  this->write_line_(command);
  this->command_state_.started();
}

void Sim800LDataComponent::http_get(const std::string &url) {
  if (this->http_state_.state == HttpState::PENDING) {
    ESP_LOGE(TAG, "HTTP request pending, ignoring");
    return;
  }
  if (this->http_state_.state == HttpState::QUEUED) {
    ESP_LOGW(TAG, "HTTP request queued, overwriting");
  }
  this->http_state_ = {
      .state = HttpState::QUEUED, .method = HttpState::GET, .url = url, .status_code = 0, .response_data = ""};
  this->http_state_.ssl =
      url.size() >= strlen(HTTPS_PROTO) && strcasecmp(url.substr(0, strlen(HTTPS_PROTO)).c_str(), HTTPS_PROTO) == 0;

  ESP_LOGI(TAG, "HTTP GET queued: %s ssl=%d", url.c_str(), this->http_state_.ssl);
}

}  // namespace sim800l_data
}  // namespace esphome
