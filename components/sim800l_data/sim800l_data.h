#pragma once

#include <utility>
#include <vector>

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#include "helpers.h"

namespace esphome {
namespace sim800l_data {

constexpr uint16_t READ_BUFFER_LENGTH = 1024;
// Default timeout to wait for OK or ERROR
constexpr uint16_t DEFAULT_COMMAND_TIMEOUT = 1000;
// Default timeout to wait if an URC is required
constexpr uint16_t DEFAULT_URC_TIMEOUT = 30000;
constexpr uint16_t BEARER_OPEN_CLOSE_TIMEOUT = 10000;
constexpr char CR = 0x0D;
constexpr char LF = 0x0A;

// Debugging
constexpr uint32_t DEBUG_SLOWDOWN = 0;

enum class State : uint8_t {
  INIT,
  DISABLE_ECHO,
  CHECK_BATTERY,
  CHECK_BATTERY_RESPONSE,
  CHECK_PIN,
  CHECK_PIN_RESPONSE,
  SET_CONTYPE_GRPS,
  SET_APN,
  SET_APN_USER,
  SET_APN_PWD,
  CHECK_REGISTRATION,
  CHECK_REGISTRATION_RESPONSE,
  CHECK_SIGNAL_QUALITY,
  CHECK_SIGNAL_QUALITY_RESPONSE,
  IDLE,

  HTTP_GET_INIT,
  HTTP_GET_SET_BEARER,
  HTTP_GET_OPEN_BEARER,
  HTTP_GET_SET_URL,
  HTTP_GET_ACTION,
  HTTP_GET_ACTION_RESPONSE,
  HTTP_GET_FAILED,

  CLEANUP1,
  CLEANUP2

};

// Result of the last sent command
enum class CommandResult : uint8_t {
  // Not waiting for a response
  NONE,
  // Command succeeded
  OK,
  // Error result or timeout
  ERROR,
  // Waiting for response
  PENDING
};

class CommandState {
 public:
  bool is_pending{false};
  std::vector<std::string> response_lines;
  std::string command;
  uint8_t expected_response_lines;
  State success_state;
  State error_state;
  uint32_t timeout;
  bool urc_required;
  uint32_t urc_timeout;
  uint32_t start;
  bool ok_received;

  const std::string &line(const uint8_t index) const {
    if (index < response_lines.size()) {
      return response_lines[index];
    } else {
      static const std::string empty = "";
      return empty;
    }
  }

  uint32_t runtime() const { return millis() - start; }
};

struct WaitState {
  uint32_t start;
  uint32_t timeout;
};

struct HttpGetState {
  enum { OK, ERROR, QUEUED, PENDING } state{OK};
  std::string url;
  uint16_t status_code;
};

class Sim800LDataComponent : public uart::UARTDevice, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  void set_apn(std::string apn) { this->apn_ = std::move(apn); }
  void set_apn_user(std::string apn_user) { this->apn_user_ = std::move(apn_user); }
  void set_apn_password(std::string apn_password) { this->apn_password_ = std::move(apn_password); }
  void http_get(const std::string &url);
  void add_on_http_get_done_callback(std::function<void(uint16_t)> callback) {
    this->http_get_done_callback_.add(std::move(callback));
  }
  void add_on_http_get_failed_callback(std::function<void(void)> callback) {
    this->http_get_failed_callback_.add(std::move(callback));
  }

#ifdef USE_SENSOR
  void set_signal_strength_sensor(sensor::Sensor *sensor) { signal_strength_sensor_ = sensor; }
  void set_battery_level_sensor(sensor::Sensor *sensor) { battery_level_sensor_ = sensor; }
  void set_battery_voltage_sensor(sensor::Sensor *sensor) { battery_voltage_sensor_ = sensor; }
#endif

 protected:
#ifdef USE_SENSOR
  sensor::Sensor *signal_strength_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor_{nullptr};
  sensor::Sensor *battery_voltage_sensor_{nullptr};
#endif
  CallbackManager<void(uint16_t)> http_get_done_callback_;
  CallbackManager<void(void)> http_get_failed_callback_;

  // The current state of this component.
  State state_{State::INIT};

  // -- config --

  std::string apn_;
  std::string apn_user_;
  std::string apn_password_;

  // -- uart reading/writing --

  char read_buffer_[READ_BUFFER_LENGTH];
  size_t buffer_pos_{0};
  bool read_line_(std::string &line);
  void write_line_(const std::string &line);

  // -- wait handling --

  WaitState wait_state_;
  void wait_(const uint32_t timeout);
  bool is_waiting_();

  // -- command handling --

  /// @brief Send a command.
  /// @param command The command.
  /// @param success_state The component state after the command was executed successfully.
  /// @param error_state The component state after the command fails or times out.
  /// @param expected_response_lines The numer ob expected response lines. If it doesn't match the actual received
  /// lines, the command will error out, even if OK is received. 0 to disable the check (e.g. when unknown).
  /// @param timeout The timout in ms.
  void send_command_(const std::string &command, State success_state, State error_state,
                     uint8_t expected_response_lines, uint32_t timeout, bool urc_required, uint32_t urc_timeout);

  void send_command_(const std::string &command, State success_state) {
    send_command_(command, success_state, State::INIT, 0, DEFAULT_COMMAND_TIMEOUT, false, DEFAULT_URC_TIMEOUT);
  }

  void send_command_(const std::string &command, State success_state, State error_state) {
    send_command_(command, success_state, error_state, 0, DEFAULT_COMMAND_TIMEOUT, false, DEFAULT_URC_TIMEOUT);
  }

  void send_command_(const std::string &command, State success_state, uint8_t expected_response_lines) {
    send_command_(command, success_state, State::INIT, expected_response_lines, DEFAULT_COMMAND_TIMEOUT, false,
                  DEFAULT_URC_TIMEOUT);
  }

  void send_command_(const std::string &command, State success_state, State error_state,
                     uint8_t expected_response_lines) {
    send_command_(command, success_state, error_state, expected_response_lines, DEFAULT_COMMAND_TIMEOUT, false,
                  DEFAULT_URC_TIMEOUT);
  }

  void send_command_(const std::string &command, State success_state, State error_state,
                     uint8_t expected_response_lines, uint32_t timeout) {
    send_command_(command, success_state, error_state, expected_response_lines, timeout, false, DEFAULT_URC_TIMEOUT);
  }

  CommandResult receive_response_();
  CommandState command_state_;

  // -- http --

  HttpGetState http_state_;
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

class HttpGetDoneTrigger : public Trigger<uint16_t> {
 public:
  explicit HttpGetDoneTrigger(Sim800LDataComponent *parent) {
    parent->add_on_http_get_done_callback([this](const uint16_t status_code) { this->trigger(status_code); });
  }
};

class HttpGetFailedTrigger : public Trigger<> {
 public:
  explicit HttpGetFailedTrigger(Sim800LDataComponent *parent) {
    parent->add_on_http_get_failed_callback([this]() { this->trigger(); });
  }
};

}  // namespace sim800l_data
}  // namespace esphome
