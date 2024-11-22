# SIM800L Data Component
This ESPHome component allows you to control a SIM800L GSM module. It specifically focuses on data transmission capabilities.

ESPHome already has a SIM800L component, but it only supports calls and SMS. I tried to add data transmission to the existing component but had some problems. So I decided to write my own implementation.

The component uses the HTTP application built into the SIM800L module. For now, only HTTP GET requests are implemented, because that is all I needed for my personal projects. Additional features might be added in the future.

````
# Example configuration for an ESP8266
uart:
  baud_rate: 9600
  rx_pin:
    number: GPIO4
  tx_pin:
    number: GPIO5

external_components:
  - source: github://christianhubmann/esphome_component_sim800l_data@main
    components: [ sim800l_data ]

sim800l_data:
  pin: "1234"
  apn: "internet"
  apn_user: ""
  apn_password: ""
  update_interval: 10s
  idle_sleep: False
  on_http_request_done:
    - logger.log:
        format: "HTTP request done: %d %s"
        args: ["status_code", "response_body.c_str()"]
        level: INFO
  on_http_request_failed:
    - logger.log:
        format: "HTTP request failed"
        level: ERROR

sensor:
  - platform: sim800l_data
    signal_strength:
      name: "Signal Strength"
    battery_level:
      name: "Battery Level"
    battery_voltage:
      name: "Battery Voltage"

script:
  - id: http_get_request
    mode: restart
    then:
      - sim800l_data.http_get:
          url: !lambda |-
            return "http://www.domain.com/?value=0";
````

## Configuration variables:
- **pin (Optional)**: The PIN of the SIM card, if the SIM is locked with PIN. If the given PIN is wrong, the component will stop because after 3 tries the SIM would become locked with PUK. If the SIM is locked with PUK, you need to unlock it with another device.
- **apn (Optional)**: The APN name. Ask your SIM provider.
- **apn_user (Optional)**: The APN username.
- **apn_password (Optional)**: The APN password.
- **update_interval (Optional, Time)**: Defaults to `10s`. How often to check connection to the SIM800L module and update sensors.
- **idle_sleep (Optional)**: Defaults to `False`. When `True`, the SIM800L sleep mode is activated when the component is idle.

## http_get Action
Send a HTTP GET request to a URL. The action opens a GPRS connection, sends the requests, waits for a response and then closes the GPRS connection. While a HTTP GET request is pending, new requests will be ignored. The timeout is 30s.

````
on_...:
  then:
    - sim800l_data.http_get:
        url: "http://www.domain.com/?value=0"
````

When the URL begins with `https://`, the HTTPSSL function of the SIM800L module will be turned on. Whether your module supports HTTPSSL seems to depend on the firmware version. Also, only protocols up to TLS 1.0 seem to be supported by the latest firmware.

## on_http_request_done Trigger
This automation triggers when a HTTP request was completed successfully. This does not mean that the remote server returned a success status code, only that the request was completed. The parameter `status_code` (of type uint16_t) contains the HTTP status code. The parameter `response_body` (of type `std::string`) contains the returned data. Because device RAM is usually limited, only a maximum of 10kB of data will be returned.

````
on_http_request_done:
  - logger.log:
      format: "HTTP request done: %d %s"
      args: ["status_code", "response_body.c_str()"]
      level: INFO
````

## on_http_request_failed Trigger
This automation tirggers when a HTTP request could not be completed, e.g. because of network problems.
````
on_http_request_failed:
  - logger.log:
      format: "HTTP request failed"
      level: ERROR
````
