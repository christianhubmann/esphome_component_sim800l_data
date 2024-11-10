#include "helpers.h"

namespace esphome {
namespace sim800l_data {

// static const char *const TAG = "sim800l_data";

void parse_response_params(const std::string &response, uint8_t param_count, std::string &p1, std::string &p2,
                           std::string &p3) {
  // read command responses take the following form: AT+XYZ: 1,2,3
  size_t start = response.find(": ");
  if (start == std::string::npos) {
    return;
  }
  start++;

  for (uint8_t i = 1; i <= param_count; i++) {
    size_t end = response.find(",", start + 1);
    if (end == std::string::npos) {
      end = response.size();
    }
    std::string p = response.substr(start + 1, end - start - 1);
    switch (i) {
      case 1:
        p1 = p;
        break;
      case 2:
        p2 = p;
        break;
      case 3:
        p3 = p;
        return;
    }
    if (end >= response.size()) {
      return;
    }
    start = end;
  }
}

void parse_response_params(const std::string &response, std::string &p1) {
  std::string p2, p3;
  parse_response_params(response, 1, p1, p2, p3);
}

void parse_response_params(const std::string &response, std::string &p1, std::string &p2) {
  std::string p3;
  parse_response_params(response, 2, p1, p2, p3);
}

void parse_response_params(const std::string &response, std::string &p1, std::string &p2, std::string &p3) {
  parse_response_params(response, 3, p1, p2, p3);
}

bool ignore_response(const std::string &response) { return response == "Call Ready" || response == "SMS Ready"; }

bool is_urc_response(const std::string &command, const std::string &response) {
  size_t length = response.find(":");
  if (length == std::string::npos) {
    return false;
  }
  // Compare substrings to avoid creating temporary strings
  static const std::string prefix = "AT";
  return command.compare(0, prefix.size(), prefix) == 0 &&
         command.compare(prefix.size(), length, response, 0, length) == 0;
}

int8_t get_rssi_dbm(uint8_t rssi_param) {
  switch (rssi_param) {
    case 2:
      return -109;
    case 3:
      return -107;
    case 4:
      return -105;
    case 5:
      return -103;
    case 6:
      return -101;
    case 7:
      return -99;
    case 8:
      return -97;
    case 9:
      return -95;
    case 10:
      return -93;
    case 11:
      return -91;
    case 12:
      return -89;
    case 13:
      return -87;
    case 14:
      return -85;
    case 15:
      return -83;
    case 16:
      return -81;
    case 17:
      return -79;
    case 18:
      return -77;
    case 19:
      return -75;
    case 20:
      return -73;
    case 21:
      return -71;
    case 22:
      return -69;
    case 23:
      return -67;
    case 24:
      return -65;
    case 25:
      return -63;
    case 26:
      return -61;
    case 27:
      return -59;
    case 28:
      return -57;
    case 29:
      return -55;
    case 30:
      return -53;
  }
  return 0;
}

}  // namespace sim800l_data
}  // namespace esphome
