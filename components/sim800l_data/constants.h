#pragma once

namespace esphome {
namespace sim800l_data {

static const char *const TAG = "sim800l_data";

static const uint16_t MAX_READ_BUFFER_SIZE = 512;
static const uint16_t SETUP_WAIT = 1000;
static const uint16_t DEFAULT_COMMAND_TIMEOUT = 1000;
static const uint16_t DEFAULT_URC_TIMEOUT = 30000;
static const uint16_t CHECK_PIN_TIMEOUT = 5000;      // according to Command Manual
static const uint32_t BEARER_OPEN_TIMEOUT = 85000;   // according to Command Manual
static const uint16_t BEARER_CLOSE_TIMEOUT = 65000;  // according to Command Manual
static const uint16_t HTTP_ACTION_TIMEOUT = 5000;    // according to Command Manual
static const uint16_t MAX_HTTP_RESPONSE_SIZE = 10240;
static const uint16_t NOT_REGISTERED_WAIT = 2000;

// The Command Manual recommends to wait 100ms after AT when sleep is enabled
static const uint16_t AT_SLEEP_WAIT = 100;

// How long to wait after errors that can't be resolved.
// Used to not spam the log with errors.
static const uint16_t FUTILE_WAIT = 5000;

static const char CR = 0x0D;
static const char LF = 0x0A;
static const char *const AT = "AT";
static const char *const OK = "OK";
static const char *const ERROR = "ERROR";
static const char *const READY = "READY";
static const char *const SIM_PIN = "SIM PIN";
static const char *const SIM_PUK = "SIM PUK";
static const char *const HTTPS_PROTO = "https:";

}  // namespace sim800l_data
}  // namespace esphome
