# SIM800L Data Component
This ESPHome component allows you to control a SIM800L GSM module. It specifically focuses on data transmission capabilities.

ESPHome already has a SIM800L component, but it only supports calls and SMS. I tried to add data transmission to the existing component but had some problems. So I decided to write my own implementation.

The component uses the HTTP application built into the SIM800L module. For that reason, only http (not https) is supported.
For now, only simple HTTP GET requests are implemented, because that is all I needed for my personal projects. Additional features might be added in the future.

````
# Example configuration for an ESP8266
uart:
  baud_rate: 9600
  rx_pin: 
    number: GPIO4
  tx_pin:
    number: GPIO5

sim800l_data:
  apn: "internet"
  apn_user: ""
  apn_password: ""
  update_interval: 10s
  on_http_get_done:
    - logger.log:
        format: "HTTP GET done: %d"
        args: ["status_code"]
        level: INFO
  on_http_get_failed:
    - logger.log:
        format: "HTTP GET failed"
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

## http_get Action
Send a HTTP GET request to a URL. The action opens a GPRS connection, sends the requests, waits for a response and then closes the GPRS connection. While a HTTP GET request is pending, new requests will be ignored. The timeout is 30s.

````
on_...:
  then:
    - sim800l_data.http_get:
        url: "http://www.domain.com/?value=0"
````

## on_http_get_done Trigger
This automation triggers when a HTTP GET request was completed successfully. This does not mean that the remote server returned a success status code, only that the request was completed. A `status_code` uint16_t parameter is provided.

````
on_http_get_done:
  - logger.log:
      format: "HTTP GET done: %d"
      args: ["status_code"]
      level: INFO
````

## on_http_get_failed Trigger
This automation tirggers when a HTTP GET request could not be completed, e.g. because of network problems.
````
on_http_get_failed:
  - logger.log:
      format: "HTTP GET failed"
      level: ERROR
````
