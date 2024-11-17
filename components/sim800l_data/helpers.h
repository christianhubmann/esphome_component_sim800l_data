#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sim800l_data {

// Parse up to p_count parameters of the response into p1, p2, p3.
void get_response_param_(const std::string &response, const uint8_t p_count, std::string &p1, std::string &p2,
                         std::string &p3);

void get_response_param(const std::string &response, std::string &p1, std::string &p2, std::string &p3);

void get_response_param(const std::string &response, std::string &p1, std::string &p2);

void get_response_param(const std::string &response, std::string &p);

template<typename T, enable_if_t<(std::is_integral<T>::value && std::is_unsigned<T>::value), int> = 0>
void get_response_param(const std::string &response, T &p1) {
  std::string s1;
  get_response_param_(response, 1, s1, s1, s1);
  p1 = parse_number<T>(s1).value_or(0);
}

template<typename T1, typename T2,
         enable_if_t<(std::is_integral<T1>::value && std::is_unsigned<T1>::value && std::is_integral<T2>::value &&
                      std::is_unsigned<T2>::value),
                     int> = 0>
void get_response_param(const std::string &response, T1 &p1, T2 &p2) {
  std::string s1, s2;
  get_response_param_(response, 2, s1, s2, s2);
  p1 = parse_number<T1>(s1).value_or(0);
  p2 = parse_number<T2>(s2).value_or(0);
}

template<typename T1, typename T2, typename T3,
         enable_if_t<(std::is_integral<T1>::value && std::is_unsigned<T1>::value && std::is_integral<T2>::value &&
                      std::is_unsigned<T2>::value && std::is_integral<T3>::value && std::is_unsigned<T3>::value),
                     int> = 0>
void get_response_param(const std::string &response, T1 &p1, T2 &p2, T3 &p3) {
  std::string s1, s2, s3;
  get_response_param_(response, 3, s1, s2, s3);
  p1 = parse_number<T1>(s1).value_or(0);
  p2 = parse_number<T2>(s2).value_or(0);
  p3 = parse_number<T3>(s3).value_or(0);
}

// Returns whether the given response is a response or URC for the given command.
bool is_response_or_urc(const std::string &command, const std::string &response);

// Converts the result parameter of +CSQ to a RSSI dBm value.
int8_t get_rssi_dbm(uint8_t rssi_param);

}  // namespace sim800l_data
}  // namespace esphome
