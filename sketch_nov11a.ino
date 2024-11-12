#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Hash.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Network credentials
const char *ssid = "Vodafone-4EA1";
const char *password = "nUFPtchh7UGMu4H4";

// Server and WebSocket setup
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);  // GMT+1

#define DHTPIN 4       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float temperature = 0.0;
float humidity = 0.0;

// Set LED GPIO
const int redPin = 14;
const int bluePin = 0;
const int warmPin = 5;
const int coolPin = 2;

// Relay pins
const int relay1Pin = 12;  // Relay 1
const int relay2Pin = 13;  // Relay 2
const int relay3Pin = 15;  // Relay 3

// Slider Switch State
bool sliderSwitchState = false;

// Initial relay states
bool relay1State = false;
bool relay2State = false;
bool relay3State = false;

String message = "";
// Initial slider switch states
bool redSliderState = true;
bool blueSliderState = true;
bool warmSliderState = true;
bool coolSliderState = true;

// Slider values
String redSlider = "0";
String blueSlider = "0";
String warmSlider = "0";
String coolSlider = "0";

// Stored slider values
String storedRedSlider = "0";
String storedBlueSlider = "0";
String storedWarmSlider = "0";
String storedCoolSlider = "0";

int redDutyCycle;
int blueDutyCycle;
int warmDutyCycle;
int coolDutyCycle;

// Relay pin definitions
const int relay1Pin = 12;
const int relay2Pin = 13;
const int relay3Pin = 15;

// Relay states
struct RelayState {
  bool state;
  bool timerEnabled;
  int startHour;
  int startMinute;
  int endHour;
  int endMinute;
};

RelayState relay1 = { false, false, 0, 0, 0, 0 };
RelayState relay2 = { false, false, 0, 0, 0, 0 };
RelayState relay3 = { false, false, 0, 0, 0, 0 };

// Timer variables
unsigned long lastTime = 0;
const unsigned long timerDelay = 20000;

// Json Variable to Hold Slider Values
JSONVar sliderValues;

// Get Slider Values
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



// Function declarations
void initFS();
void initWiFi();
void notifyClients(const String &message);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void checkRelayTimers();
bool isTimeInRange(int currentHour, int currentMinute, const RelayState &relay);

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
  Serial.print("Connecting to WiFi ...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());
}

void notifierSlider(String sliderValues) {
  ws.textAll(sliderValues);
}

void notifierDHT(String dhtValues) {
  ws.textAll(dhtValues);
}

// Notify clients of state changes
void notifyClients(const String &message) {
  ws.textAll(message);
}

// Parse relay timer message
void parseRelayTimer(const String &message, RelayState &relay) {
  // Expected format: relayTimeX:HH:MM:HH:MM:STATE
  int firstColon = message.indexOf(':');
  int secondColon = message.indexOf(':', firstColon + 1);
  int thirdColon = message.indexOf(':', secondColon + 1);
  int fourthColon = message.indexOf(':', thirdColon + 1);
  int fifthColon = message.indexOf(':', fourthColon + 1);

  relay.startHour = message.substring(firstColon + 1, secondColon).toInt();
  relay.startMinute = message.substring(secondColon + 1, thirdColon).toInt();
  relay.endHour = message.substring(thirdColon + 1, fourthColon).toInt();
  relay.endMinute = message.substring(fourthColon + 1, fifthColon).toInt();
  relay.timerEnabled = (message.substring(fifthColon + 1).toInt() == 1);
}

// Handle WebSocket messages
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String message = String((char *)data);

    // Handle relay states
    if (message.indexOf("r1:") == 0) {
      relay1State = message.substring(3) == "1";
      digitalWrite(relay1Pin, relay1State ? HIGH : LOW);
      notifyRelayState();
    }
    if (message.indexOf("r2:") == 0) {
      relay2State = message.substring(3) == "1";
      digitalWrite(relay2Pin, relay2State ? HIGH : LOW);
      notifyRelayState();
    }
    if (message.indexOf("r3:") == 0) {
      relay3State = message.substring(3) == "1";
      digitalWrite(relay3Pin, relay3State ? HIGH : LOW);
      notifyRelayState();
    }

    // Handle slider changes (unchanged)
    if (message.indexOf("1rs") >= 0) {
      redSlider = message.substring(3);
      redDutyCycle = redSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("2rs") >= 0) {
      blueSlider = message.substring(3);
      blueDutyCycle = blueSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("3rs") >= 0) {
      warmSlider = message.substring(3);
      warmDutyCycle = warmSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("4rs") >= 0) {
      coolSlider = message.substring(3);
      coolDutyCycle = coolSlider.toInt();
      notifierSlider(getSliderValues());
    }
    if (strcmp((char *)data, "sliderValues") == 0) {
      notifierSlider(getSliderValues());
    }

    // Handle individual slider switches
    if (message.indexOf("rs1:") == 0) {
      redSliderState = message.substring(4) == "1";
      if (!redSliderState) {
        storedRedSlider = redSlider;
        redDutyCycle = 0;
      } else {
        redSlider = storedRedSlider;
        redDutyCycle = redSlider.toInt();
      }
      notifyRelayState();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("rs2:") == 0) {
      blueSliderState = message.substring(4) == "1";
      if (!blueSliderState) {
        storedBlueSlider = blueSlider;
        blueDutyCycle = 0;
      } else {
        blueSlider = storedBlueSlider;
        blueDutyCycle = blueSlider.toInt();
      }
      notifyRelayState();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("rs3:") == 0) {
      warmSliderState = message.substring(4) == "1";
      if (!warmSliderState) {
        storedWarmSlider = warmSlider;
        warmDutyCycle = 0;
      } else {
        warmSlider = storedWarmSlider;
        warmDutyCycle = warmSlider.toInt();
      }
      notifyRelayState();
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("rs4:") == 0) {
      coolSliderState = message.substring(4) == "1";
      if (!coolSliderState) {
        storedCoolSlider = coolSlider;
        coolDutyCycle = 0;
      } else {
        coolSlider = storedCoolSlider;
        coolDutyCycle = coolSlider.toInt();
      }
      notifyRelayState();
      notifierSlider(getSliderValues());
    }

    // Handle global switch (only affects slider switches)
    if (message.indexOf("rG:") == 0) {
      sliderSwitchState = message.substring(3) == "1";

      // Update all slider states
      redSliderState = sliderSwitchState;
      blueSliderState = sliderSwitchState;
      warmSliderState = sliderSwitchState;
      coolSliderState = sliderSwitchState;

      if (!sliderSwitchState) {
        // Store current values and set duty cycles to 0
        storedRedSlider = redSlider;
        storedBlueSlider = blueSlider;
        storedWarmSlider = warmSlider;
        storedCoolSlider = coolSlider;

        redDutyCycle = 0;
        blueDutyCycle = 0;
        warmDutyCycle = 0;
        coolDutyCycle = 0;
      } else {
        // Restore stored values
        redSlider = storedRedSlider;
        blueSlider = storedBlueSlider;
        warmSlider = storedWarmSlider;
        coolSlider = storedCoolSlider;

        redDutyCycle = redSlider.toInt();
        blueDutyCycle = blueSlider.toInt();
        warmDutyCycle = warmSlider.toInt();
        coolDutyCycle = coolSlider.toInt();
      }

      notifyRelayState();
      notifierSlider(getSliderValues());
    }
    if (message.startsWith("relayTime1")) {
      parseRelayTimer(message, relay1);
      notifyClients("relay1Timer:" + message);
    }
    if (message.startsWith("relayTime2")) {
      parseRelayTimer(message, relay2);
      notifyClients("relay2Timer:" + message);
    }
    if (message.startsWith("relayTime3")) {
      parseRelayTimer(message, relay3);
      notifyClients("relay3Timer:" + message);
    }
  }
}

// WebSocket event handler
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// Initialize WebSocket
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Check if current time is within relay's scheduled range
bool isTimeInRange(int currentHour, int currentMinute, const RelayState &relay) {
  if (!relay.timerEnabled) return false;

  int currentMinutes = currentHour * 60 + currentMinute;
  int startMinutes = relay.startHour * 60 + relay.startMinute;
  int endMinutes = relay.endHour * 60 + relay.endMinute;

  if (endMinutes < startMinutes) {  // Handle overnight schedules
    return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
  }

  return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
}

// Check and update relay states based on timers
void checkRelayTimers() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Check each relay
  if (isTimeInRange(currentHour, currentMinute, relay1)) {
    digitalWrite(relay1Pin, HIGH);
    relay1.state = true;
  } else if (relay1.timerEnabled) {
    digitalWrite(relay1Pin, LOW);
    relay1.state = false;
  }

  if (isTimeInRange(currentHour, currentMinute, relay2)) {
    digitalWrite(relay2Pin, HIGH);
    relay2.state = true;
  } else if (relay2.timerEnabled) {
    digitalWrite(relay2Pin, LOW);
    relay2.state = false;
  }

  if (isTimeInRange(currentHour, currentMinute, relay3)) {
    digitalWrite(relay3Pin, HIGH);
    relay3.state = true;
  } else if (relay3.timerEnabled) {
    digitalWrite(relay3Pin, LOW);
    relay3.state = false;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(warmPin, OUTPUT);
  pinMode(coolPin, OUTPUT);

  // Initialize pins
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);

  // Initialize systems
  initFS();
  initWiFi();
  initWebSocket();
  timeClient.begin();

  // Setup web server routes
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

  checkRelayTimers();

  if ((millis() - lastTime) > timerDelay) {
    Serial.println("Time: " + String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()));
    lastTime = millis();
  }

  ws.cleanupClients(); // Keep WebSocket connections active
}
