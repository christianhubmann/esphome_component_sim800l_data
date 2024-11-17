#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#include "constants.h"
#include "states.h"
#include "helpers.h"

namespace esphome {
namespace sim800l_data {

class Sim800LDataComponent : public uart::UARTDevice, public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;
  void set_apn(std::string apn) { this->apn_ = std::move(apn); }
  void set_apn_user(std::string apn_user) { this->apn_user_ = std::move(apn_user); }
  void set_apn_password(std::string apn_password) { this->apn_password_ = std::move(apn_password); }
  void http_get(const std::string &url);
  void add_on_http_request_done_callback(std::function<void(uint16_t, std::string &)> callback) {
    this->http_request_done_callback_.add(std::move(callback));
  }
  void add_on_http_request_failed_callback(std::function<void(void)> callback) {
    this->http_request_failed_callback_.add(std::move(callback));
  }
#ifdef USE_SENSOR
  void set_signal_strength_sensor(sensor::Sensor *sensor) { signal_strength_sensor_ = sensor; }
  void set_battery_level_sensor(sensor::Sensor *sensor) { battery_level_sensor_ = sensor; }
  void set_battery_voltage_sensor(sensor::Sensor *sensor) { battery_voltage_sensor_ = sensor; }
#endif

 protected:
  State state_{State::INIT};
  CommandState command_state_;
  WaitState wait_;
  HttpState http_state_;
  std::string read_buffer_;

  // Read the next response line into the read buffer.
  // Returns true when a line has been read.
  bool read_line_();

  // Read until the read buffer has up to length bytes.
  // Returns true when the read buffer contains data.
  bool read_bytes_(const uint16_t length);

  // Read incoming responses and handle them.
  // Returns false if we are waiting on something.
  bool handle_response_();

  // Write a string to UART.
  void write_(const std::string &s);

  // Write a string to UART. \r\n will be appended.
  void write_line_(const std::string &s);

  // Send a command and wait for OK.
  void await_ok_(const std::string &command, const State success_state, const State error_state,
                 const uint32_t timeout);

  // Send a command and wait for OK.
  void await_ok_(const std::string &command, const State success_state) {
    this->await_ok_(command, success_state, State::INIT, DEFAULT_COMMAND_TIMEOUT);
  }

  // Send a command and wait for OK.
  void await_ok_(const std::string &command, const State success_state, const State error_state) {
    this->await_ok_(command, success_state, error_state, DEFAULT_COMMAND_TIMEOUT);
  }

  // Send a command and wait for a response, then OK.
  void await_response_(const std::string &command, State success_state, State error_state, uint32_t timeout);

  // Send a command and wait for a response, then OK.
  void await_response_(const std::string &command, State success_state) {
    this->await_response_(command, success_state, State::INIT, DEFAULT_COMMAND_TIMEOUT);
  }

  // Send a command and wait for OK, then URC.
  void await_urc_(const std::string &command, State success_state, State error_state, uint32_t timeout,
                  uint32_t urc_timeout);

  // Send a command and wait for OK, then URC.
  void await_urc_(const std::string &command, State success_state, State error_state) {
    this->await_urc_(command, success_state, error_state, DEFAULT_COMMAND_TIMEOUT, DEFAULT_URC_TIMEOUT);
  }

  // Send a command and wait for data of a specific length, then OK.
  void await_data_(const std::string &command, uint32_t data_length, State success_state, State error_state,
                   uint32_t timeout);

  // Send a command and wait for data of a specific length, then OK.
  void await_data_(const std::string &command, uint32_t data_length, State success_state, State error_state) {
    this->await_data_(command, data_length, success_state, error_state, DEFAULT_COMMAND_TIMEOUT);
  }

#ifdef USE_SENSOR
  sensor::Sensor *signal_strength_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor_{nullptr};
  sensor::Sensor *battery_voltage_sensor_{nullptr};
#endif
  CallbackManager<void(uint16_t, std::string &)> http_request_done_callback_;
  CallbackManager<void(void)> http_request_failed_callback_;
  std::string apn_;
  std::string apn_user_;
  std::string apn_password_;
};

template<typename... Ts> class HttpGetAction : public Action<Ts...> {
 public:
  HttpGetAction(Sim800LDataComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, url)

  void play(Ts... x) {
    auto url = this->url_.value(x...);
    this->parent_->http_get(url);
  }

 protected:
  Sim800LDataComponent *parent_;
};

class HttpRequestDoneTrigger : public Trigger<uint16_t, std::string &> {
 public:
  explicit HttpRequestDoneTrigger(Sim800LDataComponent *parent) {
    parent->add_on_http_request_done_callback(
        [this](uint16_t status_code, std::string &response_body) { this->trigger(status_code, response_body); });
  }
};

class HttpRequestFailedTrigger : public Trigger<> {
 public:
  explicit HttpRequestFailedTrigger(Sim800LDataComponent *parent) {
    parent->add_on_http_request_failed_callback([this]() { this->trigger(); });
  }
};

}  // namespace sim800l_data
}  // namespace esphome
