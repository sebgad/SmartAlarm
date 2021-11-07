# SmartAlarm
Alarm for ESP32 device

### required aduino libaries:
- for WiFi connection [WiFi](https://www.arduino.cc/en/Reference/WiFi)
- for SPIFFS filesystem [SPIFFS](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPIFFS)
- for Json handling [ArduinoJson](https://arduinojson.org/)

### required Addons for Arduino IDE:
- SPIFFS data upload [SPIFFS-Data-upload](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)

In addition there is one file required for storing the configuration. It needs to be stored under ./data/configuration.json. Please see the following example:

```json
{
"wifi": {
	"SSID": "SSID_12345",
	"Password": "PW_12345"
	}
}
```
