#include "helpers.h"

namespace esphome {
namespace sim800l_data {

void get_response_param_(const std::string &response, const uint8_t p_count, std::string &p1, std::string &p2,
                         std::string &p3) {
  // Expample response: +CBC: 1,2,3
  size_t start = response.find(": ");
  if (start == std::string::npos) {
    return;
  }
  start++;

  for (uint8_t i = 1; i <= p_count; i++) {
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

void get_response_param(const std::string &response, std::string &p1, std::string &p2, std::string &p3) {
  get_response_param_(response, 3, p1, p2, p3);
}

void get_response_param(const std::string &response, std::string &p1, std::string &p2) {
  get_response_param_(response, 2, p1, p2, p2);
}

void get_response_param(const std::string &response, std::string &p) { get_response_param_(response, 1, p, p, p); }

bool is_response_or_urc(const std::string &command, const std::string &response) {
  size_t length = response.find(":");
  if (length == std::string::npos) {
    return false;
  }
  return command.compare(0, length, response, 0, length) == 0;
}

int8_t get_rssi_dbm(const uint8_t rssi_param) {
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

const std::string str_concat(const std::string &s1, const std::string &s2, const std::string &s3) {
  std::string result;
  result.reserve(s1.size() + s2.size() + s3.size());
  result.append(s1);
  result.append(s2);
  result.append(s3);
  return result;
}

}  // namespace sim800l_data
}  // namespace esphome
