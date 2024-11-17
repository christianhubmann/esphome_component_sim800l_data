#pragma once

namespace esphome {
namespace sim800l_data {

constexpr char TAG[] = "sim800l_data";

constexpr uint16_t MAX_READ_BUFFER_SIZE = 512;
constexpr uint16_t DEFAULT_COMMAND_TIMEOUT = 1000;
constexpr uint16_t DEFAULT_URC_TIMEOUT = 30000;
constexpr uint16_t BEARER_OPEN_CLOSE_TIMEOUT = 10000;
constexpr uint16_t ERROR_WAIT = 1000;
constexpr uint16_t MAX_HTTP_RESPONSE_SIZE = 10240;
constexpr uint16_t AT_SLEEP_WAIT = 100;

constexpr char CR = 0x0D;
constexpr char LF = 0x0A;
constexpr char AT[] = "AT";
constexpr char ATE0[] = "ATE0";
constexpr char OK[] = "OK";
constexpr char ERROR[] = "ERROR";
constexpr char READY[] = "READY";
constexpr char HTTPS_PROTO[] = "https:";

}  // namespace sim800l_data
}  // namespace esphome
