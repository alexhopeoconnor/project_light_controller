#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#define WEBSERVER_H TRUE // Prevents compilation issues with ESPAsyncWebServer
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

#define DEBUG
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

// Define input/output pins
#define LDR_INPUT_PIN A0
#define BTN_INPUT_PIN D5
#define LED1_OUTPUT_PIN D1
#define LED2_OUTPUT_PIN D4

// Define behaviour constants
#define DEVICE_NAME "BedroomProjectAreaLights"
#define BUTTON_SHORT_DELAY 50 // ms
#define BUTTON_LONG_DELAY 500 // ms

// Web server
AsyncWebServer server(80);

// Global variables
bool turnedOn = false;

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

String templateProcessor(const String& var) {
  return var;
}

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void sendResponseCode(AsyncWebServerRequest *request, int code, const String& content) {
  AsyncWebServerResponse *response = request->beginResponse(code, MIME_HTML, content);
  response->addHeader("Access-Control-Allow-Origin", "*");
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
      response->addHeader("Access-Control-Allow-Origin", "*");
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
  return loadFromFS(request, path, MIME_JAVASCRIPT, true);
}

bool setupServer() {
  // Mount SPIFFS
  if(SPIFFS.begin()) {
    // Setup the web server page handlers
    server.onNotFound(handleNotFound);
    //server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { loadHTMLFromFS(request, "/index.html"); });

    // Setup other resource handlers
    //server.on("/static/css/main.924db34f.css", HTTP_GET, [](AsyncWebServerRequest *request) { loadCSSFromFS(request, "/static/css/main.924db34f.css"); });
    //server.on("/static/js/main.a355f3d3.js", HTTP_GET, [](AsyncWebServerRequest *request) { loadJSFromFS(request, "/static/js/main.a355f3d3.js"); });
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
    // server.on("/gate-status", HTTP_GET, [] (AsyncWebServerRequest *request) { loadJSONFromFS(request, "/gate-status.json"); });
    // server.on("/network-status", HTTP_GET, [] (AsyncWebServerRequest *request) { loadJSONFromFS(request, "/network-status.json"); });
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) { 
      turnedOn = true;
      sendResponseCode(request, 200, "Turned on.");
    });
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) { 
      turnedOn = false;
      sendResponseCode(request, 200, "Turned off.");
    });
    // server.on("/close-gate", HTTP_GET, [](AsyncWebServerRequest *request) { 
    //   sendGateSerialCommand("cg", NULL);
    //   sendResponseCode(request, 200, "Closing the gate.");
    // });
    // server.on("/stop-gate", HTTP_GET, [](AsyncWebServerRequest *request) { 
    //   sendGateSerialCommand("sg", NULL);
    //   sendResponseCode(request, 200, "Stopping the gate.");
    // });
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

  // Setup IO pins
  setupIOPins();
  
  // Initialize station mode
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_NAME);

  // Initialize WiFi
  WiFiManager wifiManager;
  wifiManager.setConnectTimeout(180);
  bool wifiManagerAutoConnected = wifiManager.autoConnect(DEVICE_NAME);

  // Restart if connection fails
  if(!wifiManagerAutoConnected) {
      ESP.restart();
      return;
  }
  WiFi.setAutoReconnect(true);

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