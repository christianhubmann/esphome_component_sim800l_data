#include "sim800l_data.h"
#include "helpers.h"

namespace esphome {
namespace sim800l_data {

static const char *const TAG = "sim800l_data";

void Sim800LDataComponent::setup() {
  // wait a bit before initialization starts
  this->state_ = State::INIT;
  this->wait_(1000);
}

void Sim800LDataComponent::update() {
  // do nothing if we are waiting, or a command is pending
  if (this->is_waiting_() || this->command_state_.is_pending) {
    return;
  }

  // if we are idle, restart all checks
  if (this->state_ == State::IDLE) {
    this->state_ = State::INIT;
  }
}

void Sim800LDataComponent::loop() {
  CommandResult result = this->receive_response_();
  if (result == CommandResult::PENDING) {
    // a command is still executing, wait for it to finish or time out
    return;
  }

  if (result == CommandResult::OK) {
    this->state_ = this->command_state_.success_state;
  } else if (result == CommandResult::ERROR) {
    // On error, restart initialization.
    // For AT command, do it immediately, otherwise wait a bit
    if (this->command_state_.error_state != State::INIT) {
      this->state_ = this->command_state_.error_state;
    } else {
      this->state_ = State::INIT;
      this->wait_(1000);
    }
    // TODO: use CLEANUP here
  }

  // do nothing if we are waiting
  if (is_waiting_()) {
    return;
  }

  // Send command. While command execution is pending, we will not reach this
  // point again; only after a command succeeded or failed.
  // When it succeeds, the state is set to the given next_state.
  // When it fails, the state is reset to INIT.
  switch (this->state_) {
    case State::INIT:
      this->send_command_("AT", State::DISABLE_ECHO);
      break;

    case State::DISABLE_ECHO:
      this->send_command_("ATE0", State::CHECK_BATTERY);
      break;

    case State::CHECK_BATTERY:
      this->send_command_("AT+CBC", State::CHECK_BATTERY_RESPONSE, 1);
      break;

    case State::CHECK_BATTERY_RESPONSE: {
      uint8_t bcs, percent;
      uint16_t voltage;
      parse_response_params(this->command_state_.line(0), bcs, percent, voltage);
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
      this->send_command_("AT+CPIN?", State::CHECK_PIN_RESPONSE, 1);
      break;

    case State::CHECK_PIN_RESPONSE: {
      std::string code;
      parse_response_params(this->command_state_.line(0), code);
      if (code == "READY") {
        this->state_ = State::SET_CONTYPE_GRPS;
      } else {
        // TODO: PIN config not yet implemented
        this->state_ = State::INIT;
        this->wait_(5000);
      }
      break;
    }

    case State::SET_CONTYPE_GRPS:
      this->send_command_("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", State::SET_APN);
      break;

    case State::SET_APN:
      this->send_command_("AT+SAPBR=3,1,\"APN\",\"" + this->apn_ + "\"", State::SET_APN_USER);
      break;

    case State::SET_APN_USER:
      this->send_command_("AT+SAPBR=3,1,\"USER\",\"" + this->apn_user_ + "\"", State::SET_APN_PWD);
      break;

    case State::SET_APN_PWD:
      this->send_command_("AT+SAPBR=3,1,\"PWD\",\"" + this->apn_password_ + "\"", State::CHECK_REGISTRATION);
      break;

    case State::CHECK_REGISTRATION:
      this->send_command_("AT+CREG?", State::CHECK_REGISTRATION_RESPONSE, 1);
      break;

    case State::CHECK_REGISTRATION_RESPONSE: {
      uint8_t n, stat;
      parse_response_params(this->command_state_.line(0), n, stat);
      if (stat != 1 && stat != 5) {
        ESP_LOGE(TAG, "Not registered to network.");
        this->state_ = State::INIT;
        this->wait_(5000);
      } else {
        this->state_ = State::CHECK_SIGNAL_QUALITY;
      }
    } break;

    case State::CHECK_SIGNAL_QUALITY:
      this->send_command_("AT+CSQ", State::CHECK_SIGNAL_QUALITY_RESPONSE, 1);
      break;

    case State::CHECK_SIGNAL_QUALITY_RESPONSE: {
      uint8_t rssi;
      parse_response_params(this->command_state_.line(0), rssi);
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
      // If there is a pending http request, start it now. Otherwise do nothing.
      if (this->http_state_.state == HttpGetState::QUEUED) {
        this->state_ = State::HTTP_GET_INIT;
      }
      break;

    case State::HTTP_GET_INIT:
      // Ignore failure. Assume that HTTP is already initialized.
      // If it's not, the next command will fail anyway.
      this->http_state_.state = HttpGetState::PENDING;
      this->send_command_("AT+HTTPINIT", State::HTTP_GET_SET_BEARER, State::HTTP_GET_SET_BEARER);
      break;

    case State::HTTP_GET_SET_BEARER:
      this->send_command_("AT+HTTPPARA=\"CID\",1", State::HTTP_GET_OPEN_BEARER, State::HTTP_GET_FAILED);
      break;

    case State::HTTP_GET_OPEN_BEARER:
      this->send_command_("AT+SAPBR=1,1", State::HTTP_GET_SET_URL, State::HTTP_GET_FAILED, 0,
                          BEARER_OPEN_CLOSE_TIMEOUT);
      break;

    case State::HTTP_GET_SET_URL:
      this->send_command_("AT+HTTPPARA=\"URL\",\"" + this->http_state_.url + "\"", State::HTTP_GET_ACTION,
                          State::HTTP_GET_FAILED);
      break;

    case State::HTTP_GET_ACTION:
      this->send_command_("AT+HTTPACTION=0", State::HTTP_GET_ACTION_RESPONSE, State::HTTP_GET_FAILED, 1,
                          DEFAULT_COMMAND_TIMEOUT, true, DEFAULT_URC_TIMEOUT);
      break;

    case State::HTTP_GET_ACTION_RESPONSE: {
      uint8_t method;
      uint16_t status_code;
      parse_response_params(this->command_state_.line(0), method, status_code);
      this->http_state_.state = HttpGetState::OK;
      this->http_state_.status_code = status_code;
      ESP_LOGI(TAG, "HTTP GET succeeded: %d %s", status_code, this->http_state_.url.c_str());
      this->http_get_done_callback_.call(status_code);
      this->state_ = State::CLEANUP1;
    } break;

    case State::HTTP_GET_FAILED:
      ESP_LOGE(TAG, "HTTP GET failed: %s", this->http_state_.url.c_str());
      this->http_state_.state = HttpGetState::ERROR;
      this->state_ = State::CLEANUP1;
      this->http_get_failed_callback_.call();
      break;

    case State::CLEANUP1:
      this->send_command_("AT+HTTPTERM", State::CLEANUP2, State::CLEANUP2);
      break;
    case State::CLEANUP2:
      this->send_command_("AT+SAPBR=0,1", State::IDLE, State::INIT, 0, BEARER_OPEN_CLOSE_TIMEOUT);
      break;
  }
}

void Sim800LDataComponent::dump_config() {}

void Sim800LDataComponent::http_get(const std::string &url) {
  if (this->http_state_.state == HttpGetState::PENDING) {
    ESP_LOGE(TAG, "HTTP GET pending, ignoring");
    return;
  }
  if (this->http_state_.state == HttpGetState::QUEUED) {
    ESP_LOGW(TAG, "HTTP GET already queued, overwriting");
  }
  this->http_state_ = {.state = HttpGetState::QUEUED, .url = url, .status_code = 0};
  ESP_LOGI(TAG, "HTTP GET queued: %s", url.c_str());
}

bool Sim800LDataComponent::read_line_(std::string &line) {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    // ESP_LOGVV(TAG, "--> %02X", byte);

    // read buffer is full, start at the beginning
    if (this->buffer_pos_ >= READ_BUFFER_LENGTH) {
      this->buffer_pos_ = 0;
    }

    if (byte == CR) {
      continue;
    } else if (byte == LF) {
      line = std::string(this->read_buffer_, this->buffer_pos_);
      if (line == "") {
        // ignore empty lines
        continue;
      }
      ESP_LOGV(TAG, "--> %s", line.c_str());
      this->buffer_pos_ = 0;
      return true;
    } else {
      this->read_buffer_[this->buffer_pos_] = byte;
      this->buffer_pos_++;
    }
  }
  return false;
}

void Sim800LDataComponent::write_line_(const std::string &line) {
  ESP_LOGV(TAG, "<-- %s", line.c_str());
  this->write_str(line.c_str());
  this->write_byte(CR);
  this->write_byte(LF);
}

void Sim800LDataComponent::send_command_(const std::string &command, State success_state, State error_state,
                                         uint8_t expected_response_lines, uint32_t timeout, bool urc_required,
                                         uint32_t urc_timeout) {
  if (this->command_state_.is_pending) {
    ESP_LOGE(TAG, "Command \"%s\" is pending, ignoring", this->command_state_.command.c_str());
    return;
  }

  this->command_state_ = {.is_pending = true,
                          .response_lines = {},
                          .command = command,
                          .expected_response_lines = expected_response_lines,
                          .success_state = success_state,
                          .error_state = error_state,
                          .timeout = timeout,
                          .urc_required = urc_required,
                          .urc_timeout = urc_timeout,
                          .start = 0,
                          .ok_received = false};

  // shrink the vector after receiving large responses (like reading SMS)
  if (expected_response_lines > 0 && this->command_state_.response_lines.capacity() > expected_response_lines) {
    this->command_state_.response_lines.shrink_to_fit();
  }

  this->write_line_(command);
  this->command_state_.start = millis();

  if (DEBUG_SLOWDOWN > 0 && this->wait_state_.timeout == 0) {
    this->wait_(DEBUG_SLOWDOWN);
  }
}

bool is_command_timeout(const CommandState &command_state) {
  if (!command_state.is_pending) {
    return false;
  }

  const uint32_t runtime = command_state.runtime();
  const bool ok_timeout = !command_state.ok_received && runtime > command_state.timeout;
  const bool urc_timeout = !ok_timeout && command_state.urc_required && runtime > command_state.urc_timeout;
  if (!ok_timeout && !urc_timeout) {
    return false;
  }

  ESP_LOGE(TAG, "Command timed out after %d ms", runtime);
  return true;
}

bool check_response_line_count(const CommandState &command_state) {
  if (command_state.expected_response_lines == 0) {
    return true;
  }
  if (command_state.expected_response_lines == command_state.response_lines.size()) {
    return true;
  }

  ESP_LOGE(TAG, "Command \"%s\" expected %d response lines, but got %d after %d ms", command_state.command.c_str(),
           command_state.expected_response_lines, command_state.response_lines.size(), command_state.runtime());

  return false;
}

CommandResult Sim800LDataComponent::receive_response_() {
  std::string resp;
  bool read = this->read_line_(resp);

  if (read) {
    if (ignore_response(resp)) {
      if (this->command_state_.is_pending) {
        ESP_LOGV(TAG, "Ignoring: \"%s\"", resp.c_str());
        return CommandResult::PENDING;
      }
      return CommandResult::NONE;
    }

    if (!this->command_state_.is_pending) {
      ESP_LOGW(TAG, "Received unexpected response, ignoring: \"%s\"", resp.c_str());
      return CommandResult::NONE;
    }

    if (resp == "OK") {
      this->command_state_.ok_received = true;

      if (!this->command_state_.urc_required && !check_response_line_count(this->command_state_)) {
        this->command_state_.is_pending = false;
        return CommandResult::ERROR;
      }

      if (this->command_state_.urc_required) {
        ESP_LOGI(TAG, "Command \"%s\" OK, waiting for URC", this->command_state_.command.c_str());
        return CommandResult::PENDING;
      }

      ESP_LOGI(TAG, "Command \"%s\" succeeded after %d ms", this->command_state_.command.c_str(),
               this->command_state_.runtime());
      this->command_state_.is_pending = false;
      return CommandResult::OK;
    }

    if (resp == "ERROR") {
      ESP_LOGE(TAG, "Command failed after %d ms", this->command_state_.runtime());
      this->command_state_.is_pending = false;
      return CommandResult::ERROR;
    }

    // TODO: check for CME ERROR

    this->command_state_.response_lines.push_back(resp);

    if (this->command_state_.urc_required && is_urc_response(this->command_state_.command, resp)) {
      if (!check_response_line_count(this->command_state_)) {
        return CommandResult ::ERROR;
      }

      ESP_LOGI(TAG, "Command \"%s\" succeeded  after %d ms", this->command_state_.command.c_str(),
               this->command_state_.runtime());
      this->command_state_.is_pending = false;
      return CommandResult::OK;
    }

    return CommandResult::PENDING;
  }

  // timeout check
  if (this->command_state_.is_pending) {
    if (is_command_timeout(this->command_state_)) {
      this->command_state_.is_pending = false;
      return CommandResult::ERROR;
    }
    return CommandResult::PENDING;
  }
  return CommandResult::NONE;
}

void Sim800LDataComponent::wait_(const uint32_t timeout) {
  ESP_LOGVV(TAG, "Wait for %d ms", timeout);
  this->wait_state_.timeout = timeout;
  this->wait_state_.start = millis();
}

bool Sim800LDataComponent::is_waiting_() {
  if (this->wait_state_.timeout != 0) {
    if ((millis() - this->wait_state_.start) < this->wait_state_.timeout) {
      return true;
    }
    this->wait_state_.timeout = 0;
    ESP_LOGVV(TAG, "Wait finished");
  }
  return false;
}

}  // namespace sim800l_data
}  // namespace esphome
