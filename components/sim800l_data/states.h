#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include "constants.h"

namespace esphome {
namespace sim800l_data {

enum class State : uint8_t {
  INIT,
  DISABLE_ECHO,
  DISABLE_SLEEP,
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
  ENABLE_SLEEP,

  HTTP_INIT,
  HTTP_SET_SSL,
  HTTP_SET_BEARER,
  HTTP_OPEN_BEARER,
  HTTP_SET_URL,
  HTTP_ACTION,
  HTTP_ACTION_RESPONSE,
  HTTP_READ_RESPONSE,
  HTTP_FAILED,
  HTTP_TERM,
  HTTP_CLOSE_BEARER
};

class CommandState {
 public:
  std::string command;
  State success_state;
  State error_state;
  uint32_t timeout;
  uint32_t urc_timeout;
  bool ok_received;
  bool response_required;
  std::string response;
  bool urc_required;
  std::string urc;
  uint32_t data_required;
  std::string data;
  bool is_pending;
  uint32_t start;

  void reset(const std::string &command = "", State success_state = State::INIT, State error_state = State::INIT,
             uint32_t timeout = DEFAULT_COMMAND_TIMEOUT);

  uint32_t runtime() const { return millis() - start; }

  bool response_received() const { return !this->response.empty(); }

  bool data_complete() const { return this->data.size() == this->data_required; }

  bool urc_received() const { return !this->urc.empty(); }

  void started() {
    this->is_pending = true;
    this->start = millis();
  }

  bool timed_out() const;
};

class WaitState {
 protected:
  uint32_t started_;
  uint32_t timeout_;

 public:
  // Wait before the next command is executed.
  void start(const uint32_t timeout);

  // Update the wait state. Returns true if still waiting.
  bool is_waiting();
};

class HttpState {
 public:
  enum { NONE, QUEUED, PENDING } state{NONE};
  enum { GET } method{GET};
  bool ssl;
  std::string url;
  uint16_t status_code;
  std::string response_data;

  void reset();
};

}  // namespace sim800l_data
}  // namespace esphome
