****************************************************************
**Basic ESP8266 code with GPIO, MQTT and stdout.**

Works and tested on ESP8266-01

****************************************************************
**Features**
****************************************************************

Reads GPIO swtich status
Interupt driven key switih reader on GPIO0. (switch must be on high when booting.)

Publishes on /ESP-01/GPIO00 with "1" or "0".

Switch triggers - subcribes to /ESP-01/GPIO02 with "1" or "0" to trigger GPIO2.

Publishes "hello" on /ESP-01/status on boot

LWT on boot as "offline" on /ESP-01/status

Workaround fix for my ESP8266-01 not loading the SSID from flash.

****************************************************************

**GPIO code from SDK Example folder + various sources on the net.**

****************************************************************
**esp8266_stdout**
****************************************************************


****************************************************************

**MQTT code based on https://github.com/tuanpmt/esp_mqtt**

****************************************************************

# esp8266-PIR
