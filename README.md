# SmartAlarm
Alarm for ESP32 device

### required aduino libaries:
- for WiFi connection [WiFi](https://www.arduino.cc/en/Reference/WiFi)
- for Json handling [ArduinoJson](https://arduinojson.org/)
- for LittleFS file partition (https://github.com/espressif/arduino-esp32/tree/master/libraries/LittleFS)
- for ntp functionality [time](https://github.com/espressif/arduino-esp32/tree/master/libraries)

### required Addons for Arduino IDE:
- File system data upload tool [Data-upload](https://github.com/me-no-dev/arduino-esp32fs-plugin)

In addition there is one file required for storing the configuration. It needs to be stored under ./data/configuration.json. Please see the following example:

```json
{
"wifi": {
	"SSID": "SSID_12345",
	"Password": "PW_12345"
	},
"ntp":{
	"ServerUrl": "europe.pool.ntp.org",
	"GmtOffset": 3600,
	"DaylightSavingTime": 3600
}
}
```
