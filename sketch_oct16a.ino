#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Hash.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char *ssid = "";
const char *password = "";

#define DHTPIN 4  // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float temperature = 0.0;
float humidity = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Set LED GPIO
const int redPin = 0;
const int bluePin = 2;
const int warmPin = 14;
const int coolPin = 5;

// Relay pins
const int relay1Pin = 12;  // Relay 1
const int relay2Pin = 13;  // Relay 2
const int relay3Pin = 15;  // Relay 3

// Global Switch State
bool globalSwitchState = false;

// Initial relay states
bool relay1State = false;
bool relay2State = false;
bool relay3State = false;

String message = "";
String redSlider = "0";
String blueSlider = "0";
String warmSlider = "0";
String coolSlider = "0";

int redDutyCycle;
int blueDutyCycle;
int warmDutyCycle;
int coolDutyCycle;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;

//Json Variable to Hold Slider Values
JSONVar sliderValues;

//Get Slider Values
String getSliderValues() {
  sliderValues["redSlider"] = String(redSlider);
  sliderValues["blueSlider"] = String(blueSlider);
  sliderValues["warmSlider"] = String(warmSlider);
  sliderValues["coolSlider"] = String(coolSlider);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}


JSONVar dhtValues;

String getDhtValues() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  dhtValues["temperature"] = String(temperature);
  dhtValues["humidity"] = String(humidity);

  String jsonString = JSON.stringify(dhtValues);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifierSlider(String sliderValues) {
  ws.textAll(sliderValues);
}

void notifierDHT(String dhtValues) {
  ws.textAll(dhtValues);
}

void notifyRelayState() {
  // Send current relay states as 1 or 0
  JSONVar relayStates;
  relayStates["relay1"] = relay1State ? "1" : "0";
  relayStates["relay2"] = relay2State ? "1" : "0";
  relayStates["relay3"] = relay3State ? "1" : "0";
  relayStates["globalSwitch"] = globalSwitchState ? "1" : "0";

  String relayStateMessage = JSON.stringify(relayStates);
  ws.textAll(relayStateMessage);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char *)data;

    // Handle relay states
    if (message.indexOf("relay1:") == 0) {
      relay1State = message.substring(7) == "1";
      digitalWrite(relay1Pin, relay1State ? HIGH : LOW);
    }
    if (message.indexOf("relay2:") == 0) {
      relay2State = message.substring(7) == "1";
      digitalWrite(relay2Pin, relay2State ? HIGH : LOW);
    }
    if (message.indexOf("relay3:") == 0) {
      relay3State = message.substring(7) == "1";
      digitalWrite(relay3Pin, relay3State ? HIGH : LOW);
    }
    if (message.indexOf("globalSwitch:") == 0) {
      globalSwitchState = message.substring(13) == "1";
      relay1State = globalSwitchState;
      relay2State = globalSwitchState;
      relay3State = globalSwitchState;
      digitalWrite(relay1Pin, relay1State ? HIGH : LOW);
      digitalWrite(relay2Pin, relay2State ? HIGH : LOW);
      digitalWrite(relay3Pin, relay3State ? HIGH : LOW);
      notifyRelayState();
    }

    // Handle slider changes (unchanged)
    if (message.indexOf("1s") >= 0) {
      redSlider = message.substring(2);
      redDutyCycle = redSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("2s") >= 0) {
      blueSlider = message.substring(2);
      blueDutyCycle = blueSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("3s") >= 0) {
      warmSlider = message.substring(2);
      warmDutyCycle = warmSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("4s") >= 0) {
      coolSlider = message.substring(2);
      coolDutyCycle = coolSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (strcmp((char *)data, "sliderValues") == 0) {
      notifierSlider(getSliderValues());
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyRelayState();  // Send relay states to the newly connected client
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
      break;
    case WS_EVT_ERROR:
      Serial.printf("WebSocket error with client #%u\n", client->id());
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(warmPin, OUTPUT);
  pinMode(coolPin, OUTPUT);

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);

  initFS();
  initWiFi();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
}

void loop() {
  analogWrite(redPin, redDutyCycle);
  analogWrite(bluePin, blueDutyCycle);
  analogWrite(warmPin, warmDutyCycle);
  analogWrite(coolPin, coolDutyCycle);

  if ((millis() - lastTime) > timerDelay) {
    String sliderValues = getSliderValues();
    String dhtValues = getDhtValues();

    notifierDHT(dhtValues);

    lastTime = millis();
  }

  ws.cleanupClients();  // Keep WebSocket connections active
}
