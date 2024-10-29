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

// Replace with your network credentials
const char *ssid = "GuestWlan-UpperFloor";
const char *password = "RF-47475!";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);  // GMT+1 as Beispiel

#define DHTPIN 4       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float temperature = 0.0;
float humidity = 0.0;

// Set LED GPIO
const int redPin = 0;
const int bluePin = 2;
const int warmPin = 14;
const int coolPin = 5;

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

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;

bool relay1ScheduledOn = false;
bool relay2ScheduledOn = false;
bool relay3ScheduledOn = false;

int relay1StartHour = 0;
int relay1StartMinute = 0;
int relay1EndHour = 0;
int relay1EndMinute = 0;

int relay2StartHour = 0;
int relay2StartMinute = 0;
int relay2EndHour = 0;
int relay2EndMinute = 0;

int relay3StartHour = 0;
int relay3StartMinute = 0;
int relay3EndHour = 0;
int relay3EndMinute = 0;

// Add variables for timer enabled states
bool relay1TimerEnabled = false;
bool relay2TimerEnabled = false;
bool relay3TimerEnabled = false;

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

// Modified checkRelayTimers function
void checkRelayTimers() {
        if (!timeClient.update()) {  // Only proceed if time update successful
            return;
        }
        
        int currentHour = timeClient.getHours();
        int currentMinute = timeClient.getMinutes();
        
        // Helper function to convert time to minutes for easier comparison
        auto timeToMinutes = [](int hours, int minutes) {
            return hours * 60 + minutes;
        };
        
        int currentTimeInMinutes = timeToMinutes(currentHour, currentMinute);
        
        // Check relay 1
        if (relay1TimerEnabled) {
            int startTime = timeToMinutes(relay1StartHour, relay1StartMinute);
            int endTime = timeToMinutes(relay1EndHour, relay1EndMinute);
            
            bool shouldBeOn = currentTimeInMinutes >= startTime && currentTimeInMinutes < endTime;
            
            if (shouldBeOn != relay1State) {
                relay1State = shouldBeOn;
                digitalWrite(relay1Pin, shouldBeOn ? HIGH : LOW);
                notifyRelayState();
            }
        }

  // Check relay 2
  if (relay2TimerEnabled) {
    int startTime = timeToMinutes(relay2StartHour, relay2StartMinute);
    int endTime = timeToMinutes(relay2EndHour, relay2EndMinute);

    if (currentTimeInMinutes >= startTime && currentTimeInMinutes < endTime) {
      if (!relay2State) {
        relay2State = true;
        digitalWrite(relay2Pin, HIGH);
        notifyRelayState();
      }
    } else {
      if (relay2State) {
        relay2State = false;
        digitalWrite(relay2Pin, LOW);
        notifyRelayState();
      }
    }
  }

  // Check relay 3
  if (relay3TimerEnabled) {
    int startTime = timeToMinutes(relay3StartHour, relay3StartMinute);
    int endTime = timeToMinutes(relay3EndHour, relay3EndMinute);

    if (currentTimeInMinutes >= startTime && currentTimeInMinutes < endTime) {
      if (!relay3State) {
        relay3State = true;
        digitalWrite(relay3Pin, HIGH);
        notifyRelayState();
      }
    } else {
      if (relay3State) {
        relay3State = false;
        digitalWrite(relay3Pin, LOW);
        notifyRelayState();
      }
    }
  }
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

String padNumber(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

// Replace the notifyRelayState function
void notifyRelayState() {
    JSONVar relayStates;
    // Only send switch states here
    relayStates["relay1State"] = relay1State ? "1" : "0";
    relayStates["relay2State"] = relay2State ? "1" : "0";
    relayStates["relay3State"] = relay3State ? "1" : "0";
    
    String relayStateMessage = JSON.stringify(relayStates);
    ws.textAll(relayStateMessage);
    
    // Send timer settings separately
    JSONVar timerStates;
    timerStates["relay1Timer"] = padNumber(relay1StartHour) + ":" + 
                                padNumber(relay1StartMinute) + ":" +
                                padNumber(relay1EndHour) + ":" +
                                padNumber(relay1EndMinute) + ":" +
                                (relay1TimerEnabled ? "1" : "0");
    
    String timerStateMessage = JSON.stringify(timerStates);
    ws.textAll(timerStateMessage);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char *)data;

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

    // Handle slider changes
    if (message.indexOf("1s") >= 0) {
      redSlider = message.substring(2);
      redDutyCycle = redSliderState ? redSlider.toInt() : 0;
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("2s") >= 0) {
      blueSlider = message.substring(2);
      blueDutyCycle = blueSliderState ? blueSlider.toInt() : 0;
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("3s") >= 0) {
      warmSlider = message.substring(2);
      warmDutyCycle = warmSliderState ? warmSlider.toInt() : 0;
      notifierSlider(getSliderValues());
    }
    if (message.indexOf("4s") >= 0) {
      coolSlider = message.substring(2);
      coolDutyCycle = coolSliderState ? coolSlider.toInt() : 0;
      notifierSlider(getSliderValues());
    }

    // Handle relay time settings
    if (message.startsWith("relayTime1:")) {
    String timeStr = message.substring(10);
    int idx1 = timeStr.indexOf(':');
    int idx2 = timeStr.indexOf(':', idx1 + 1);
    int idx3 = timeStr.indexOf(':', idx2 + 1);
    int idx4 = timeStr.indexOf(':', idx3 + 1);

    relay1StartHour = timeStr.substring(0, idx1).toInt();
    relay1StartMinute = timeStr.substring(idx1 + 1, idx2).toInt();
    relay1EndHour = timeStr.substring(idx2 + 1, idx3).toInt();
    relay1EndMinute = timeStr.substring(idx3 + 1, idx4).toInt();
    bool newTimerState = timeStr.substring(idx4 + 1) == "1";
    
    if (relay1TimerEnabled != newTimerState) {
        relay1TimerEnabled = newTimerState;
        if (!relay1TimerEnabled) {
            relay1State = false;
            digitalWrite(relay1Pin, LOW);
        }
    }

    JSONVar relayTimes;
    relayTimes["relay1"] = padNumber(relay1StartHour) + ":" + 
                          padNumber(relay1StartMinute) + ":" +
                          padNumber(relay1EndHour) + ":" +
                          padNumber(relay1EndMinute) + ":" +
                          (relay1TimerEnabled ? "1" : "0");
    String jsonString = JSON.stringify(relayTimes);
    ws.textAll(jsonString);
    }
    // Similar handlers for relay 2 and 3
    else if (message.startsWith("relayTime2:")) {
      // Similar parsing for relay 2
      String timeStr = message.substring(10);
      int idx1 = timeStr.indexOf(':');
      int idx2 = timeStr.indexOf(':', idx1 + 1);
      int idx3 = timeStr.indexOf(':', idx2 + 1);
      int idx4 = timeStr.indexOf(':', idx3 + 1);

      relay2StartHour = timeStr.substring(0, idx1).toInt();
      relay2StartMinute = timeStr.substring(idx1 + 1, idx2).toInt();
      relay2EndHour = timeStr.substring(idx2 + 1, idx3).toInt();
      relay2EndMinute = timeStr.substring(idx3 + 1, idx4).toInt();
      relay2TimerEnabled = timeStr.substring(idx4 + 1) == "1";

      JSONVar relayTimes;
      relayTimes["relay2"] = message.substring(10);
      String jsonString = JSON.stringify(relayTimes);
      ws.textAll(jsonString);
    } else if (message.startsWith("relayTime3:")) {
      // Similar parsing for relay 3
      String timeStr = message.substring(10);
      int idx1 = timeStr.indexOf(':');
      int idx2 = timeStr.indexOf(':', idx1 + 1);
      int idx3 = timeStr.indexOf(':', idx2 + 1);
      int idx4 = timeStr.indexOf(':', idx3 + 1);

      relay3StartHour = timeStr.substring(0, idx1).toInt();
      relay3StartMinute = timeStr.substring(idx1 + 1, idx2).toInt();
      relay3EndHour = timeStr.substring(idx2 + 1, idx3).toInt();
      relay3EndMinute = timeStr.substring(idx3 + 1, idx4).toInt();
      relay3TimerEnabled = timeStr.substring(idx4 + 1) == "1";

      JSONVar relayTimes;
      relayTimes["relay3"] = message.substring(10);
      String jsonString = JSON.stringify(relayTimes);
      ws.textAll(jsonString);
    }

    // Check if the message requests slider values
    if (message == "sliderValues") {
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

  checkRelayTimers();

  if ((millis() - lastTime) > timerDelay) {
    notifierDHT(getDhtValues());
    lastTime = millis();
  }

  ws.cleanupClients();  // Keep WebSocket connections active
}
