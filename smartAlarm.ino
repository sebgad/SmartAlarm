/*
 * smartAlarm
 * Controlling an alarm clock based on ESP32
 * 
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>
#include <LittleFS.h>

// File system definitions
#define FORMAT_LittleFS_IF_FAILED true
#define JSON_MEMORY 500 //bytes
#define TIME_STMP_SIZE 50

// define configuration structure type
struct config {
  String wifiSSID;
  String wifiPassword;
  String NtpServerUrl;
  int NtpGmtOffset;
  int NtpDaylightSavingTime;
};

// define configuration object
config objConfig;

// config file location
const char* strParamFilePath = "/configuration.json";
boolean bParamFileLocked = false;

// measurement file location
const char* strMeasFilePath = "/data.csv";

// WifiConnectionStatus
boolean bEspOnline = false;

// global variabel for WiFi strength
int iDbmPercentage = 0;

// timestamp object
struct tm objTimeInfo;
char strTimeInfo[TIME_STMP_SIZE];

bool connectWiFi(const int i_total_fail = 3, const int i_timout_attemp = 1000){
  /**
   * Try to connect to WiFi Accesspoint based on the given information in the header file WiFiAccess.h.
   * A defined number of connection is performed.
   * @param 
   *    i_timout_attemp:     Total amount of connection attemps
   *    i_waiting_time:   Waiting time between connection attemps
   * 
   * @return 
   *    b_status:         true if connection is successfull
   */
  
  bool b_successful = false;
  
  Serial.print("Device ");
  Serial.print(WiFi.macAddress());

  Serial.print(" try connecting to ");
  Serial.println(objConfig.wifiSSID);
  delay(100);

  int i_run_cnt_fail = 0;
  int i_wifi_status;

  WiFi.mode(WIFI_STA);

  // Connect to WPA/WPA2 network:
  WiFi.begin(objConfig.wifiSSID.c_str(), objConfig.wifiPassword.c_str());
  i_wifi_status = WiFi.status();

  while ((i_wifi_status != WL_CONNECTED) && (i_run_cnt_fail<i_total_fail)) {
    // wait for connection establish
    delay(i_timout_attemp);
    i_run_cnt_fail++;
    i_wifi_status = WiFi.status();
    Serial.print("Connection Attemp: ");
    Serial.println(i_run_cnt_fail);
  }

  if (i_wifi_status == WL_CONNECTED) {
    // Print ESP32 Local IP Address
    Serial.print("Connection successful. Local IP: ");
    Serial.println(WiFi.localIP());
    
    // Signal strength and approximate conversion to percentage
    int i_dBm = WiFi.RSSI();
    calcWifiStrength(i_dBm);
    Serial.print("Signal Strength: ");
    Serial.print(i_dBm);
    Serial.print(" dB -> ");
    Serial.print(iDbmPercentage);
    Serial.println(" %");
    b_successful = true;
  } else {
    // Wifi connection not possible, create Soft AP
    Serial.print("Connection unsuccessful. WiFi status: ");
    Serial.println(i_wifi_status);
    Serial.print("Make Soft-AP with SSID 'SmartAlarm'");
    WiFi.softAP("SmartAlarm");
  }
  
  return b_successful;
} // connectWiFi

void reconnectWiFi(WiFiEvent_t event, WiFiEventInfo_t info){
  /**
   * Try to reconnect to WiFi when disconnected from network
   */
    
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  connectWiFi(3, 1000);
} // reconnectWiFi

void calcWifiStrength(int i_dBm){
  /**
   * calculate the wifi strength from dB to % 
   */
  iDbmPercentage = 0;
  if (i_dBm>=-50) {
    iDbmPercentage = 100;
  } else if (i_dBm<=-100) {
    iDbmPercentage = 0;
  } else {
    iDbmPercentage = 2*(i_dBm+100);
  }
}//getWifiStrength

bool loadConfiguration(){
  /**
   * @brief Load configuration from JSON configuration file
   */
  
  bool b_success = false;

  if (!bParamFileLocked){
    // file is not locked by another process ->  save to read or write
    bParamFileLocked = true;
    File obj_param_file = LittleFS.open(strParamFilePath, "r");
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
      objConfig.NtpServerUrl = json_doc["ntp"]["ServerUrl"].as<String>(); // issue #118 in ArduinoJson
      objConfig.NtpGmtOffset = json_doc["ntp"]["GmtOffset"];
      objConfig.NtpDaylightSavingTime = json_doc["ntp"]["DaylightSavingTime"];
  }
    b_success = true;
  } else {
    b_success = false;
  }
  return b_success;
} // loadConfiguration


bool saveConfiguration(){
  /**
   * @brief Save configuration to configuration file 
   */
  
  bool b_success = false;
  StaticJsonDocument<JSON_MEMORY> json_doc;

  json_doc["wifi"]["SSID"] = objConfig.wifiSSID;
  json_doc["wifi"]["SSID"] = objConfig.wifiPassword;
  json_doc["ntp"]["ServerUrl"] = objConfig.NtpServerUrl;
  json_doc["ntp"]["GmtOffset"] = objConfig.NtpGmtOffset;
  json_doc["ntp"]["DaylightSavingTime"] = objConfig.NtpDaylightSavingTime;  

  if (!bParamFileLocked){
    bParamFileLocked = true;
    File obj_param_file = LittleFS.open(strParamFilePath, "w");
  
    if (serializeJsonPretty(json_doc, obj_param_file) == 0) {
      Serial.println(F("Failed to write configuration to file"));
    } else {
      Serial.println("configuration file updated successfully");
      b_success = true;
    }
    obj_param_file.close();
    bParamFileLocked = false;
  } else {
    b_success = false;
  }
  return b_success;
} // saveConfiguration

void resetConfiguration(boolean b_safe_to_json){
  /**
   * @brief init configuration to configuration file 
   * @param b_safe_to_json: Safe initial configuration to json file
   */
  
  objConfig.wifiSSID = "";
  objConfig.wifiPassword = "";
  objConfig.NtpServerUrl = "";
  objConfig.NtpGmtOffset = 0;
  objConfig.NtpDaylightSavingTime = 0;
  
  if (b_safe_to_json){
    saveConfiguration();
  }
} // resetConfiguration


bool connectNTP(){
  /**
   * @brief Connect to NTP server and set up time object
   * @return true if connection to NTP server was successful
   */
  
  char char_timestamp[50];
  bool b_success = false;

  // config time library and setup NTP settings
  Serial.println("Setting up NTP client:");
  Serial.print("Local GMT: ");
  Serial.print(objConfig.NtpGmtOffset);
  Serial.print(" s; Daylight saving time: ");
  Serial.print(objConfig.NtpDaylightSavingTime);
  Serial.print(" s; NTP server address: ");
  Serial.println(objConfig.NtpServerUrl.c_str());

  configTime(objConfig.NtpGmtOffset, objConfig.NtpDaylightSavingTime, objConfig.NtpServerUrl.c_str());

  // get local time
  if(!getLocalTime(&objTimeInfo)){
    Serial.println("Failed to obtain time stamp online");
  } else {
    b_success = true;
    Serial.print("NTP setup successfull. Local time is now: ");
    // write time stamp into variable
    strftime(char_timestamp, sizeof(char_timestamp), "%c", &objTimeInfo);
    Serial.println(char_timestamp);
  }
  
  return b_success;
} // connectNTP

void getLatestTimeInfo(char * ptr_time_info, size_t i_time_info_size) {
 /**
  * @brief Update timestamp and return latest time info
  * 
  */
  
  if(!getLocalTime(&objTimeInfo)){
    Serial.println("Failed to obtain time stamp online");
  } else {
    strftime(ptr_time_info, i_time_info_size, "%y/%m/%d %H:%M:%S", &objTimeInfo);
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

  // Initialize LittleFS
  if(!LittleFS.begin(FORMAT_LittleFS_IF_FAILED)){
    // Initialization of LittleFS failed, restart it
    Serial.println("LittleFS mount Failed, restart ESP");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("LittleFS mount successfully.");
    // initialize configuration before load json file
    resetConfiguration(false);

    if (loadConfiguration()) {
      // load configuration from file in eeprom
      
      // Connect to wifi when configuration can be loaded succesfully
      bEspOnline = connectWiFi(5, 2000);

      if (bEspOnline == true) {
        // ESP has wifi connection

        // configure NTP server and get actual time
        connectNTP();
      } 
      
    } else {
        // load configuration unsuccessful
        Serial.println("Parameter file is locked on startup. Please reset to factory settings.");
    }
    
    // Initialization successful
    unsigned int i_total_bytes = LittleFS.totalBytes();
    unsigned int i_used_bytes = LittleFS.usedBytes();
    
    Serial.println("");
    Serial.println("File system info:");
    Serial.print("Total space on LittleFS: ");
    Serial.print(i_total_bytes);
    Serial.println(" bytes");

    Serial.print("Total space used on LittleFS: ");
    Serial.print(i_used_bytes);
    Serial.println(" bytes");
    Serial.println("");

    if (!LittleFS.exists(strMeasFilePath)){
      // Create measurement file if not exists
      File obj_meas_file = LittleFS.open(strMeasFilePath, "w");
      obj_meas_file.print("Measurement File created on ");
      getLatestTimeInfo(strTimeInfo, TIME_STMP_SIZE);
      obj_meas_file.println(strTimeInfo);
      obj_meas_file.println("");
      obj_meas_file.println("Time,Temperature,Airpressure");
      obj_meas_file.close();
    }

  }
} // setup


void loop()
{
  // add some dummy print out to console
  getLatestTimeInfo(strTimeInfo, TIME_STMP_SIZE);
  Serial.println(strTimeInfo);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
} // loop
