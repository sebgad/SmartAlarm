/*********
 * 
 * smartAlarm
 * Controlling an alarm clock based on ESP32
 * 
*********/

#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// File system definitions
#define FORMAT_SPIFFS_IF_FAILED true
#define JSON_MEMORY 500 //bytes

// define config structure for online calibration
struct config {
  String wifiSSID;
  String wifiPassword;
};

// define configuration object
config objConfig;

// config file location
const char* strParamFilePath = "/configuration.json";
boolean bParamFileLocked = false;

// WifiConnectionStatus
boolean bEspOnline = false;


bool connectWiFi(const int i_total_fail, const int i_timout_attemp_millis){
  /**
   * Try to connect to WiFi Accesspoint based on the given information in the header file WiFiAccess.h.
   * A defined number of connection is performed.
   * @param 
   *    i_total_fail:     Total amount of connection attemps
   *    i_timout_attemp_millis:             Waiting time between connection attemps
   * 
   * @return 
   *    b_status:         true if connection is successfull
   */
  
  bool b_successful = false;
  
  WiFi.disconnect(true);
  delay(100);

  Serial.print("Device ");
  Serial.print(WiFi.macAddress());

  Serial.print(" try connecting to ");
  Serial.println(objConfig.wifiSSID);
  delay(100);

  int i_run_cnt_fail = 0;
  int i_wifi_status = WL_IDLE_STATUS;

  WiFi.mode(WIFI_STA);

  // Connect to WPA/WPA2 network:
  WiFi.begin(objConfig.wifiSSID.c_str(), objConfig.wifiPassword.c_str());

  while ((i_wifi_status != WL_CONNECTED) && (i_run_cnt_fail<i_total_fail)) {
    // wait for connection establish
    delay(i_timout_attemp_millis);
    i_run_cnt_fail++;
    i_wifi_status = WiFi.status();
  }

  if (i_wifi_status == WL_CONNECTED) {
      // Print ESP32 Local IP Address
      Serial.print("Connection successful. Local IP: ");
      Serial.println(WiFi.localIP());
      // Signal strength and approximate conversion to percentage
      int i_dBm = WiFi.RSSI();
      int i_dBm_percentage = 0;
      if (i_dBm>=-50) {
        i_dBm_percentage = 100;
      } else if (i_dBm<=-100) {
        i_dBm_percentage = 0;
      } else {
        i_dBm_percentage = 2*(i_dBm+100);
      }
      Serial.print("Signal Strength: ");
      Serial.print(i_dBm);
      Serial.print(" dB -> ");
      Serial.print(i_dBm_percentage);
      Serial.println(" %");
      b_successful = true;
  } else {
    Serial.print("Connection unsuccessful. WiFi status: ");
    Serial.println(i_wifi_status);
  }
  
  return b_successful;
} // connectWiFi

void reconnectWiFi(WiFiEvent_t event, WiFiEventInfo_t info){
  /**
   * Try to reconnect to WiFi when disconnected from network
   */
    
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  connectWiFi(3, 1000);
} // reconnectWiFi

bool loadConfiguration(){
  /**
   * Load configuration from configuration file 
   */
  
  bool b_success = false;

  if (!bParamFileLocked){
    // file is not locked by another process ->  save to read or write
    bParamFileLocked = true;
    File obj_param_file = SPIFFS.open(strParamFilePath, "r");
    StaticJsonDocument<JSON_MEMORY> json_doc;
    DeserializationError error = deserializeJson(json_doc, obj_param_file);

    if ((!obj_param_file) || (error)){
      Serial.println(F("Failed to read file, using default configuration"));
      
      if (!obj_param_file) {
        Serial.println(F("File could not be opened"));
      }
    
      if (error) {
        Serial.print(F("json deserializion error: "));
        Serial.println(error.c_str());
      }
    
      obj_param_file.close();
      bParamFileLocked = false;
      resetConfiguration(true);
    } else {
      // file could be read without issues and json document could be interpreted
      bParamFileLocked = false;
      objConfig.wifiSSID = json_doc["wifi"]["SSID"].as<String>(); // issue #118 in ArduinoJson
      objConfig.wifiPassword = json_doc["wifi"]["Password"].as<String>(); // issue #118 in ArduinoJson
  }
    b_success = true;
  } else {
    b_success = false;
  }
  return b_success;
}

bool saveConfiguration(){
  /**
   * Save configuration to configuration file 
   */
  
  bool b_success = false;
  StaticJsonDocument<JSON_MEMORY> json_doc;

  json_doc["wifi"]["SSID"] = objConfig.wifiSSID;
  json_doc["wifi"]["SSID"] = objConfig.wifiPassword;

  if (!bParamFileLocked){
    bParamFileLocked = true;
    File obj_param_file = SPIFFS.open(strParamFilePath, "w");
  
    if (serializeJsonPretty(json_doc, obj_param_file) == 0) {
      Serial.println(F("Failed to write configuration to file"));
    }
    else{
      Serial.println("configuration file updated successfully");
      b_success = true;
    }
    obj_param_file.close();
    bParamFileLocked = false;
  } else {
    b_success = false;
  }
  return b_success;
}

void resetConfiguration(boolean b_safe_to_json){
  /**
   * init configuration to configuration file 
   * 
   * @param b_safe_to_json: Safe initial configuration to json file
   */
  
  objConfig.wifiSSID = "";
  objConfig.wifiPassword = "";
  
  if (b_safe_to_json){
    saveConfiguration();
  }
}

void setup(){
  /**
  * @brief Setup smartAlarm system before running main task loop()
  */
  
  // Serial port for debugging purposes
  Serial.begin(115200);
  delay(50);
  Serial.println("Starting setup.");

  // Initialize SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    // Initialization of SPIFFS failed, restart it
    Serial.println("SPIFFS mount Failed, restart ESP");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("SPIFFS mount successfully.");
    // initialize configuration before load json file
    resetConfiguration(false);

    if (loadConfiguration()) {
      // load configuration from file in eeprom
      
      // Connect to wifi when configuration can be loaded succesfully
      bEspOnline = connectWiFi(3, 1000);

      if (bEspOnline == true) {
        // ESP has wifi connection

        // Define reconnect action when disconnecting from Wifi
        WiFi.onEvent(reconnectWiFi, SYSTEM_EVENT_STA_DISCONNECTED);
      } 
      
    } else {
        // load configuration unsuccessful
        // #TODO: Define further actions / diagnosis
        Serial.println("Parameter file is locked on startup. Please reset to factory settings.");
    }
    
    // Initialization successful
    unsigned int i_total_bytes = SPIFFS.totalBytes();
    unsigned int i_used_bytes = SPIFFS.usedBytes();
    
    Serial.println("File system info:");
    Serial.print("   Total space on SPIFFS: ");
    Serial.print(i_total_bytes);
    Serial.println(" bytes");

    Serial.print("   Total space used on SPIFFS: ");
    Serial.print(i_used_bytes);
    Serial.println(" bytes");
    Serial.println("");
  }
}

void loop()
{
	
}
