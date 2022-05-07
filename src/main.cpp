#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)     Serial.print (x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x) 
#endif

// Define behaviour constants
#define CONFIGURATION_AP_NAME "RoomProjectAreaLights"

void setup() {
  #ifdef DEBUG
  // Initialize serial
  Serial.begin(9600);
  #endif
  
  // Initialize station mode
  WiFi.mode(WIFI_STA);

  // Initialize WiFi
  bool wifiManagerAutoConnected;
  WiFiManager wifiManager;
  wifiManagerAutoConnected = wifiManager.autoConnect(CONFIGURATION_AP_NAME);

  // Restart if connection fails
  if(!wifiManagerAutoConnected) {
      ESP.restart();
      return;
  }

  // Setup OTA
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    DEBUG_PRINTLN("OTA: Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINT("Progress: ");
    DEBUG_PRINT(progress / (total / 100));
    DEBUG_PRINTLN("%");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINT("Error[");
    DEBUG_PRINT(error);
    DEBUG_PRINT("]: ");
    if (error == OTA_AUTH_ERROR) {
      DEBUG_PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      DEBUG_PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      DEBUG_PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      DEBUG_PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      DEBUG_PRINTLN("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();
}