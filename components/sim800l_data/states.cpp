#include "states.h"
namespace esphome {
namespace sim800l_data {

void CommandState::reset(const std::string &command, State success_state, State error_state, uint32_t timeout) {
  this->command = command;
  this->success_state = success_state;
  this->error_state = error_state;
  this->timeout = timeout;
  this->urc_timeout = DEFAULT_URC_TIMEOUT;
  this->ok_received = false;
  this->response_required = false;
  this->response.clear();
  this->response.shrink_to_fit();
  this->urc_required = false;
  this->urc.clear();
  this->data_required = 0;
  this->data.clear();
  this->data.shrink_to_fit();
  this->is_pending = false;
  this->start = 0;
}

bool CommandState::timed_out() const {
  const uint32_t runtime = this->runtime();
  if (!this->ok_received && runtime > this->timeout) {
    return true;
  }
  if (this->urc_required && runtime > this->urc_timeout && !this->urc_received()) {
    return true;
  }
  return false;
}

void WaitState::start(const uint32_t timeout) {
  ESP_LOGV(TAG, "Wait for %d ms", timeout);
  this->timeout_ = timeout;
  this->started_ = millis();
}

bool WaitState::is_waiting() {
  if (this->timeout_ != 0) {
    const uint32_t runtime = millis() - this->started_;
    if (runtime < this->timeout_) {
      return true;
    }
    this->timeout_ = 0;
    ESP_LOGV(TAG, "Wait finished");
  }
  return false;
}

void HttpState::reset() {
  this->state = NONE;
  this->url.clear();
  this->url.shrink_to_fit();
  this->status_code = 0;
  this->response_data.clear();
  this->response_data.shrink_to_fit();
}

}  // namespace sim800l_data
}  // namespace esphome
