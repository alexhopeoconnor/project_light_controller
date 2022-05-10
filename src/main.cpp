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

// Define input/output pins
#define LDR_INPUT_PIN A0
#define BTN_INPUT_PIN D5
#define LED1_OUTPUT_PIN D1
#define LED2_OUTPUT_PIN D4

// Define behaviour constants
#define CONFIGURATION_AP_NAME "RoomProjectLights"
#define BUTTON_SHORT_DELAY 50 // ms
#define BUTTON_LONG_DELAY 500 // ms

// Global variables
bool turnedOn = false;

void setup() {
  #ifdef DEBUG
  // Initialize serial
  Serial.begin(9600);
  #endif

  // Initialize pins
  pinMode(LDR_INPUT_PIN, INPUT);
  pinMode(BTN_INPUT_PIN, INPUT_PULLUP);
  pinMode(LED1_OUTPUT_PIN, OUTPUT);
  pinMode(LED2_OUTPUT_PIN, OUTPUT);

  // Set initial output state
  analogWriteFreq(690);
  analogWrite(LED1_OUTPUT_PIN, 0);
  analogWrite(LED2_OUTPUT_PIN, 0);

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

void onButtonLongPress() {

}

void onButtonShortPress() {
  turnedOn = !turnedOn;
}

void processButton() {
  static int buttonValue, buttonLastValue;
  static bool buttonShortPress, buttonLongPress;
  static unsigned long lastButtonPress;

  // Read input button state
  buttonLastValue = buttonValue;
  buttonValue = digitalRead(BTN_INPUT_PIN);
  if(buttonValue != buttonLastValue && buttonValue == LOW) {
    lastButtonPress = millis();
  }

  // Determine/perform button action
  if(buttonValue == LOW) {
    if((lastButtonPress + BUTTON_LONG_DELAY) <= millis()) {
      buttonLongPress = true;
      buttonShortPress = false;
    } else if((lastButtonPress + BUTTON_SHORT_DELAY) <= millis()) {
      buttonLongPress = false;
      buttonShortPress = true;
    }
  }
  else
  {
    // Check press flags and perform actions
    if(buttonLongPress) {
      onButtonLongPress();
    }
    else if(buttonShortPress) {
      onButtonShortPress();
    }
    
    // Reset press flag state
    buttonLongPress = false;
    buttonShortPress = false;
  }
}

void processLights() {
  static int step = 1;
  static bool lastOnState = false;
  static unsigned int lastBrightness = 0;
  static unsigned long lastStep = millis();

  if(lastOnState != turnedOn) {
    if(turnedOn) {
      analogWrite(LED1_OUTPUT_PIN, 255);
      analogWrite(LED2_OUTPUT_PIN, 255);
    } else {
      analogWrite(LED1_OUTPUT_PIN, 0);
      analogWrite(LED2_OUTPUT_PIN, 0);
    }
    lastOnState = turnedOn;
  }

  // Step the LED brightness
  // if(millis() > (lastStep + 50)) {
  //   analogWrite(LED1_OUTPUT_PIN, lastBrightness);
  //   analogWrite(LED2_OUTPUT_PIN, lastBrightness);
  //   lastBrightness += step;
  //   if(lastBrightness <= 0 || lastBrightness >= 255) {
  //     step *= -1;
  //   }
  //   lastStep = millis();
  // }
}

void loop() {
  // Process button
  processButton();

  // Process lights
  processLights();

  // Handle OTA updates
  ArduinoOTA.handle();
}