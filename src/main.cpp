#include <FS.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#define WEBSERVER_H TRUE // Prevents compilation issues with ESPAsyncWebServer
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "timezone.h"

//#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)     Serial.print (x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x) 
#endif

// MIME definitions
#define MIME_HTML "text/html"
#define MIME_CSS "text/css"
#define MIME_PNG "image/png"
#define MIME_ICO "image/vnd.microsoft.icon"
#define MIME_XML "application/xml"
#define MIME_JSON "application/json"
#define MIME_JAVASCRIPT "application/json"

// HTTP STRINGS
#define PARAM_BRIGHTNESS "brightness"

// Define input/output pins
#define LDR_INPUT_PIN A0
#define BTN_INPUT_PIN D5
#define LED1_OUTPUT_PIN D1
#define LED2_OUTPUT_PIN D4

// Define behaviour constants
#define DEVICE_NAME "BedroomSpotlights"
#define OTA_FLASH_DELAY 1000
#define BUTTON_SHORT_DELAY 50 // ms
#define BUTTON_LONG_DELAY 500 // ms

// Define operation modes
enum OperationMode {
  OffMode,
  OnMode,
  AutoOnOffMode,
  TimerOffMode
};
char* LEDModeNames[] = { "Off", "On", "Auto", "TimerOff" };

// Config definition
#define CONFIG_START 0
#define CONFIG_VERSION "V3"
#define PROJECT_NAME_SIZE 24 // Max host name length is 24
typedef struct
{
  char version[5];
  char project_name[PROJECT_NAME_SIZE];
  double brightness;
  OperationMode opMode;
} configuration_type;

// Global configuration instance
configuration_type CONFIGURATION = {
  CONFIG_VERSION,
  DEVICE_NAME,
  80.0,
  OffMode
};

// Web server
AsyncWebServer server(80);

// NPT
const char *TIME_SERVER = "pool.ntp.org";
int myTimeZone = AET;
time_t now;

// Global variables
bool turnedOn = false;
double brightness = 0.0;
OperationMode opMode = OffMode;

double getLightLevelPercentage() {
  const int numVals = 10;
  static int values[numVals];
  static int lastValueIndex = 0;
  static int actualValueCount = 0;

  // Read the raw value
  values[lastValueIndex] = analogRead(LDR_INPUT_PIN);
  lastValueIndex++;
  if(lastValueIndex >= numVals) {
    lastValueIndex = 0;
  }
  if(actualValueCount < numVals) {
    actualValueCount++;
  }

  // Average the raw value
  double rawValue = 0;
  for(int i = 0; i < actualValueCount; i++) {
    rawValue += values[i];
  }
  rawValue = rawValue / actualValueCount;

  // Return percentage
  return (rawValue / 1023) * 100.0;
}

unsigned int getBrightnessPWM() {
  return 255 * (brightness / 100.0);
}

void turnLightsOff() {
  analogWrite(LED1_OUTPUT_PIN, 0);
  analogWrite(LED2_OUTPUT_PIN, 0);
  turnedOn = false;
  brightness = 0.0;
}

void turnLightsOn() {
  turnedOn = true;
  brightness = CONFIGURATION.brightness;
  analogWrite(LED1_OUTPUT_PIN, getBrightnessPWM());
  analogWrite(LED2_OUTPUT_PIN, getBrightnessPWM());
}

int loadConfig() 
{
  // Check the version, load (overwrite) the local configuration struct
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
    for (unsigned int i = 0; i < sizeof(CONFIGURATION); i++) {
      *((char*)&CONFIGURATION + i) = EEPROM.read(CONFIG_START + i);
    }
    return 1;
  }
  return 0;
}

void saveConfig() 
{
  // save the CONFIGURATION in to EEPROM
  for (unsigned int i = 0; i < sizeof(CONFIGURATION); i++) {
    EEPROM.write(CONFIG_START + i, *((char*)&CONFIGURATION + i));
  }
  EEPROM.commit();
}

void setupIOPins() {
  // Initialize pins
  pinMode(LDR_INPUT_PIN, INPUT);
  pinMode(BTN_INPUT_PIN, INPUT_PULLUP);
  pinMode(LED1_OUTPUT_PIN, OUTPUT);
  pinMode(LED2_OUTPUT_PIN, OUTPUT);

  // Set initial output state
  analogWriteFreq(690);
  analogWrite(LED1_OUTPUT_PIN, 0);
  analogWrite(LED2_OUTPUT_PIN, 0);
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    DEBUG_PRINTLN("OTA: Start updating " + type);
    turnLightsOff();
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA: End");
    turnLightsOff();
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

String templateProcessor(const String& var) {
  if(var == "PROJECT_NAME") {
    return CONFIGURATION.project_name;
  } else if(var == "TURNED_ON") {
    return turnedOn ? "true" : "false";
  } else if(var == "BRIGHTNESS") {
    return String(brightness, 2);
  } else if(var == "LIGHT_LEVEL") {
    return String(getLightLevelPercentage(), 2);
  }
  return var;
}

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void addResponseHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Headers", "*");
  response->addHeader("Access-Control-Allow-Credentials", "true");
  response->addHeader("Access-Control-Allow-Methods", "GET,PUT,POST,DELETE");
}

void sendResponseCode(AsyncWebServerRequest *request, int code, const String& content) {
  AsyncWebServerResponse *response = request->beginResponse(code, MIME_HTML, content);
  addResponseHeaders(response);
  request->send(response);
}

bool loadFromFS(AsyncWebServerRequest *request, const String& path, const String& dataType, bool templateResponse) {
  // Check the specified path
  if (SPIFFS.exists(path)) {

    // Check the file can be read
    File dataFile = SPIFFS.open(path, "r");
    if (!dataFile) {
        handleNotFound(request);
        return false;
    }
    // Send the response
    if(templateResponse) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, dataType, false, templateProcessor);
      addResponseHeaders(response);
      request->send(response);
    } else {
      request->send(SPIFFS, path, dataType);
    }
    dataFile.close();
  } else {
      handleNotFound(request);
      return false;
  }
  return true;
}

bool loadHTMLFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_HTML, true);
}

bool loadCSSFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_CSS, false);
}

bool loadPNGFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_PNG, false);
}

bool loadICOFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_ICO, false);
}

bool loadXMLFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_XML, false);
}

bool loadJSONFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_JSON, true);
}

bool loadJSFromFS(AsyncWebServerRequest *request, const String& path) {
  return loadFromFS(request, path, MIME_JAVASCRIPT, false);
}

bool setupServer() {
  // Mount SPIFFS
  if(SPIFFS.begin()) {
    // Setup the web server page handlers
    server.onNotFound(handleNotFound);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { loadHTMLFromFS(request, "/index.html"); });

    // Setup other resource handlers
    server.on("/static/css/main.8f0491f5.css", HTTP_GET, [](AsyncWebServerRequest *request) { loadCSSFromFS(request, "/static/css/main.8f0491f5.css"); });
    server.on("/static/js/main.dc10bcbe.js", HTTP_GET, [](AsyncWebServerRequest *request) { loadJSFromFS(request, "/static/js/main.dc10bcbe.js"); });
    server.on("/browserconfig.xml", HTTP_GET, [](AsyncWebServerRequest *request) { loadXMLFromFS(request, "/browserconfig.xml"); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) { loadICOFromFS(request, "/favicon.ico"); });

    // Setup image handlers
    server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/logo.png"); });
    server.on("/android-icon-36x36.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-36x36.png"); });
    server.on("/android-icon-48x48.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-48x48.png"); });
    server.on("/android-icon-72x72.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-72x72.png"); });
    server.on("/android-icon-96x96.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-96x96.png"); });
    server.on("/android-icon-144x144.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-144x144.png"); });
    server.on("/android-icon-192x192.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/android-icon-192x192.png"); });
    server.on("/apple-icon-57x57.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-57x57.png"); });
    server.on("/apple-icon-60x60.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-60x60.png"); });
    server.on("/apple-icon-72x72.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-72x72.png"); });
    server.on("/apple-icon-76x76.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-76x76.png"); });
    server.on("/apple-icon-114x114.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-114x114.png"); });
    server.on("/apple-icon-120x120.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-120x120.png"); });
    server.on("/apple-icon-144x144.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-144x144.png"); });
    server.on("/apple-icon-152x152.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-152x152.png"); });
    server.on("/apple-icon-180x180.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-180x180.png"); });
    server.on("/apple-icon-precomposed.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon-precomposed.png"); });
    server.on("/apple-icon.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/apple-icon.png"); });
    server.on("/favicon-16x16.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/favicon-16x16.png"); });
    server.on("/favicon-32x32.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/favicon-32x32.png"); });
    server.on("/favicon-96x96.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/favicon-96x96.png"); });
    server.on("/ms-icon-70x70.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/ms-icon-70x70.png"); });
    server.on("/ms-icon-144x144.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/ms-icon-144x144.png"); });
    server.on("/ms-icon-150x150.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/ms-icon-150x150.png"); });
    server.on("/ms-icon-310x310.png", HTTP_GET, [](AsyncWebServerRequest *request) { loadPNGFromFS(request, "/ms-icon-310x310.png"); });

    // Setup JSON resource handlers
    server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest *request) { loadJSONFromFS(request, "/manifest.json"); });

    // Setup API handlers
    server.on("/current-status", HTTP_GET, [] (AsyncWebServerRequest *request) { loadJSONFromFS(request, "/current-status.json"); });
    server.on("/lights-on", HTTP_GET, [](AsyncWebServerRequest *request) { 
      turnLightsOn();
      sendResponseCode(request, 200, "Turned lights on.");
    });
    server.on("/lights-off", HTTP_GET, [](AsyncWebServerRequest *request) { 
      turnLightsOff();
      sendResponseCode(request, 200, "Turned lights off.");
    });
    server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request) {
      struct tm *timeinfo;
      time(&now);
      timeinfo = localtime(&now);
      int year = timeinfo->tm_year + 1900;
      int month = timeinfo->tm_mon;
      int day = timeinfo->tm_mday;
      int hour = timeinfo->tm_hour;
      int mins = timeinfo->tm_min;
      int sec = timeinfo->tm_sec;
      int day_of_week = timeinfo->tm_wday;
      String responseFormatted = "Date = " + String(day) + "/" + String(month) + "/" + String(year) + "</br>";
      responseFormatted = responseFormatted + " Time = " + String(hour) + ":" + String(mins) + ":" + String(sec) + "<br />";
      responseFormatted = responseFormatted + "Day is " + String(DAYS_OF_WEEK[day_of_week]);
      sendResponseCode(request, 200, responseFormatted);
    });
    server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
        String brightnessParam;
        if (request->hasParam(PARAM_BRIGHTNESS, true)) {
          // Parse brightness
          brightnessParam = request->getParam(PARAM_BRIGHTNESS, true)->value();
          double brightnessParsed = brightnessParam.toDouble();
          if(brightnessParsed < 1.0) {
            brightnessParsed = 1.0;
          } else if(brightnessParsed > 100.0) {
            brightnessParsed = 100.0;
          }

          // Update brightness
          if(brightnessParsed != CONFIGURATION.brightness) {
            CONFIGURATION.brightness = brightnessParsed;
          } 
          saveConfig();

          // Update current brightness
          if(turnedOn) {
            brightness = CONFIGURATION.brightness;
          }

          // Return sucessful response
          sendResponseCode(request, 200, "Brightness saved: " + String(brightness, 2));
          return;
        }

        // Return bad request response
        sendResponseCode(request, 400, "Invalid request parameters.");
    });
    server.begin();
    return true;
  }
  return false;
}

void setup() {
  #ifdef DEBUG
  // Initialize serial
  Serial.begin(9600);
  #endif

  // Initialize config
  EEPROM.begin(sizeof(CONFIGURATION));
  if(!loadConfig()) {
    saveConfig();
  }

  // Setup IO pins
  setupIOPins();
  
  // Initialize station mode
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_NAME);
  WiFi.setAutoReconnect(true);

  // Initialize WiFi
  WiFiManager wifiManager;
  wifiManager.setConnectTimeout(180);
  wifiManager.setHostname(CONFIGURATION.project_name);
  bool wifiManagerAutoConnected = wifiManager.autoConnect(CONFIGURATION.project_name);

  // Restart if connection fails
  if(!wifiManagerAutoConnected) {
      ESP.restart();
      return;
  }

  // Configure ntp
  configTime(myTimeZone * 3600, 0, TIME_SERVER);

  // Setup OTA
  setupOTA();

  // Init web server
  setupServer();
}

void onButtonLongPress() {
  // Disconnect from WiFi and restart
  WiFi.disconnect();
  ESP.restart();
}

void onButtonShortPress() {
  if(turnedOn) {
    turnLightsOff();
  } else {
    turnLightsOn();
  }
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
  static double lastBrightness = brightness;

  // Check if brightness should be updated
  if(lastBrightness != brightness) {
    // Updated brightness if lights are turned on
    if(turnedOn) {
      analogWrite(LED1_OUTPUT_PIN, getBrightnessPWM());
      analogWrite(LED2_OUTPUT_PIN, getBrightnessPWM());
    }
    lastBrightness = brightness;
  }
}

void loop() {
  // Process button
  processButton();

  // Process lights
  processLights();

  // Handle OTA updates
  ArduinoOTA.handle();
}