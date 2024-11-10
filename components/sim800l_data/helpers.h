#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sim800l_data {

void parse_response_params(const std::string &response, std::string &p1);
void parse_response_params(const std::string &response, std::string &p1, std::string &p2);
void parse_response_params(const std::string &response, std::string &p1, std::string &p2, std::string &p3);

template<typename T, enable_if_t<(std::is_integral<T>::value && std::is_unsigned<T>::value), int> = 0>
void parse_response_params(const std::string &response, T &p1) {
  std::string s1;
  parse_response_params(response, s1);
  p1 = parse_number<T>(s1).value_or(0);
}

template<typename T1, typename T2,
         enable_if_t<(std::is_integral<T1>::value && std::is_unsigned<T1>::value && std::is_integral<T2>::value &&
                      std::is_unsigned<T2>::value),
                     int> = 0>
void parse_response_params(const std::string &response, T1 &p1, T2 &p2) {
  std::string s1, s2;
  parse_response_params(response, s1, s2);
  p1 = parse_number<T1>(s1).value_or(0);
  p2 = parse_number<T2>(s2).value_or(0);
}

template<typename T1, typename T2, typename T3,
         enable_if_t<(std::is_integral<T1>::value && std::is_unsigned<T1>::value && std::is_integral<T2>::value &&
                      std::is_unsigned<T2>::value && std::is_integral<T3>::value && std::is_unsigned<T3>::value),
                     int> = 0>
void parse_response_params(const std::string &response, T1 &p1, T2 &p2, T3 &p3) {
  std::string s1, s2, s3;
  parse_response_params(response, s1, s2, s3);
  p1 = parse_number<T1>(s1).value_or(0);
  p2 = parse_number<T2>(s2).value_or(0);
  p3 = parse_number<T3>(s3).value_or(0);
}

bool ignore_response(const std::string &response);

/// @brief Returns whether the given response is an URC for the given commad.
/// @param command The command, e.g. "AT+HTTPACTION=0"
/// @param response The response, e.g. "+HTTPACTION: 0,200,34"
/// @return
bool is_urc_response(const std::string &command, const std::string &response);

// Converts the result parameter of +CSQ to a RSSI dBm value.
int8_t get_rssi_dbm(uint8_t rssi_param);

}  // namespace sim800l_data
}  // namespace esphome
