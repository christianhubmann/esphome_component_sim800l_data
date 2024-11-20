#pragma once

namespace esphome {
namespace sim800l_data {

static const char *const TAG = "sim800l_data";

static const uint16_t MAX_READ_BUFFER_SIZE = 512;
static const uint16_t DEFAULT_COMMAND_TIMEOUT = 1000;
static const uint16_t DEFAULT_URC_TIMEOUT = 30000;
static const uint16_t BEARER_OPEN_CLOSE_TIMEOUT = 10000;
static const uint16_t ERROR_WAIT = 1000;
static const uint16_t MAX_HTTP_RESPONSE_SIZE = 10240;
static const uint16_t AT_SLEEP_WAIT = 100;
static const uint16_t NOT_REGISTERED_WAIT = 5000;

static const char CR = 0x0D;
static const char LF = 0x0A;
static const char *const AT = "AT";
static const char *const OK = "OK";
static const char *const ERROR = "ERROR";
static const char *const READY = "READY";
static const char *const HTTPS_PROTO = "https:";

}  // namespace sim800l_data
}  // namespace esphome
