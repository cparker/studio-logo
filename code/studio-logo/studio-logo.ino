#include <ArduinoJson.h>
#include <FastLED.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "secrets.h"

// Create an instance of the Preferences library
Preferences preferences;

const int mqtt_port = 1883;

const char* appName = "studio-logo";

// mqtt topic for receiving mode changes
const char* mqtt_topic_mode = "studio-logo/mode";

// mqtt topic for receiving color palette changes
const char* mqtt_topic_palette = "studio-logo/palette";

// LED settings
#define NUM_LEDS 44
#define DATA_PIN 5  // Change this to the GPIO pin you've connected the LEDs to
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB myleds[NUM_LEDS];

/*
 0 default, cycle right
 1 cycle left
 2 cycle right
 3 still
*/
uint8_t mode = 0;

// some simple color definitions
CRGB blue(0, 0, 255);
CRGB green(0, 255, 0);
CRGB lighterGreen(0, 255, 127);
CRGB yellow(255, 255, 0);
CRGB red(255, 0, 0);
CRGB orange(255, 165, 0);
CRGB purple(128, 0, 128);

// set color defaults
CRGB section1Default = blue;
CRGB section2Default = green;
CRGB rightEarDefault = lighterGreen;
CRGB section3Default = yellow;
CRGB section4Default = red;
CRGB leftEarDefault = orange;
CRGB section5Default = blue;

// active colors
CRGB section1 = section1Default;
CRGB section2 = section2Default;
CRGB rightEar = rightEarDefault;
CRGB section3 = section3Default;
CRGB section4 = section4Default;
CRGB leftEar = leftEarDefault;
CRGB section5 = section5Default;

CRGB sectionColor[] = {section1, section2, rightEar, section3,
                       section4, leftEar,  section5};
uint8_t sectionNoEarsColor[] = {section1, section2, section3, section4,
                                section5};

uint32_t defaultInterval = 5000;
uint8_t defaultFade = 500;

// Arrays for section LED counts and brightness
const uint8_t sectionLedCounts[] = {8, 8, 2, 8, 8, 2, 8};

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(myleds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  randomSeed(analogRead(0));

  setupWifi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  setDefaults();
  restoreMode();
}

void restoreMode() {
  preferences.begin(appName, false);
  mode = preferences.getUChar("mode", 0);
  preferences.end();
  Serial.print("Loaded mode value from NVS: ");
  Serial.println(mode);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to a null-terminated string
  char payload_str[length + 1];
  memcpy(payload_str, payload, length);
  payload_str[length] = '\0';

  // Parse JSON data
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload_str);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (strcmp(topic, "studio-logo/palette") == 0) {
    // Parse and set the global variables for palette
    JsonArray arr;

    arr = doc["section1"];
    section1 = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["section2"];
    section2 = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["rightEar"];
    rightEar = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["section3"];
    section3 = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["section4"];
    section4 = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["leftEar"];
    leftEar = CRGB(arr[0], arr[1], arr[2]);

    arr = doc["section5"];
    section5 = CRGB(arr[0], arr[1], arr[2]);

    // Save the color palette to NVS
    preferences.begin(appName, false);
    preferences.putBytes("section1", &section1, sizeof(CRGB));
    preferences.putBytes("section2", &section2, sizeof(CRGB));
    preferences.putBytes("rightEar", &rightEar, sizeof(CRGB));
    preferences.putBytes("section3", &section3, sizeof(CRGB));
    preferences.putBytes("section4", &section4, sizeof(CRGB));
    preferences.putBytes("leftEar", &leftEar, sizeof(CRGB));
    preferences.putBytes("section5", &section5, sizeof(CRGB));
    preferences.end();
  } else if (strcmp(topic, "studio-logo/mode") == 0) {
    // Parse and set the global variable for mode
    mode = doc["mode"];

    // Save the mode to NVS
    preferences.begin(appName, false);
    preferences.putUChar("mode", mode);
    preferences.end();
  } else {
    Serial.println(F("Unknown topic"));
  }
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("connected");
      mqttClient.subscribe(mqtt_topic_mode);
      mqttClient.subscribe(mqtt_topic_palette);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setupWifi() {
  delay(10);

  // Indicate Wi-Fi connection status
  myleds[0] = CRGB::Red;
  FastLED.show();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Indicate successful Wi-Fi connection
  myleds[0] = CRGB::Green;
  FastLED.show();
  delay(5000);
}

void setSection1RGB(CRGB color) { fill_solid(myleds, 8, color); }

void setSection2RGB(CRGB color) { fill_solid(myleds + 8, 8, color); }

void setRightEarRGB(CRGB color) { fill_solid(myleds + 16, 2, color); }

void setSection3RGB(CRGB color) { fill_solid(myleds + 18, 8, color); }

void setSection4RGB(CRGB color) { fill_solid(myleds + 26, 8, color); }

void setLeftEarRGB(CRGB color) { fill_solid(myleds + 34, 2, color); }

void setSection5RGB(CRGB color) { fill_solid(myleds + 36, 8, color); }

void cycleLeft() {
  static uint32_t prevMillis = 0;
  uint32_t currentMillis = millis();
  uint32_t interval = 5000;
  uint32_t fadeDuration = 500;

  if (currentMillis - prevMillis >= interval) {
    prevMillis = currentMillis;

    // TODO:
    Serial.println("Cycling left");
  }
}

void cycleRight() {
  Serial.println("Cycling right");
  static uint32_t prevMillis = 0;
  uint32_t currentMillis = millis();
  uint32_t interval = 5000;
  uint32_t fadeDuration = 128;

  if (currentMillis - prevMillis >= interval) {
    prevMillis = currentMillis;

    // TODO:
    Serial.println("Cycling right");

    CRGB startColor1 = CRGB::Red;
    CRGB endColor1 = CRGB::Yellow;

    CRGB startColor2 = CRGB::Green;
    CRGB endColor2 = CRGB::Purple;

    uint16_t duration = 1000;  // Transition duration in milliseconds
    uint8_t numSteps = 128;    // Number of steps in the transition

    for (uint8_t step = 0; step < numSteps; step++) {
      // Transition section1 (first 8 LEDs) from red to yellow
      transitionColor(startColor1, endColor1, step, numSteps, 0, 7, myleds);

      // Transition section2 (second 8 LEDs) from green to purple
      transitionColor(startColor2, endColor2, step, numSteps, 8, 15, myleds);

      FastLED.show();
      FastLED.delay(duration / numSteps);
    }

    delay(2000);  // Pause for 2 seconds between transitions
  }
}

void transitionColor(CRGB startColor, CRGB endColor, uint8_t currentStep,
                     uint8_t numSteps, uint8_t startIndex, uint8_t endIndex,
                     CRGB* leds) {
  CRGB currentColor =
      startColor.lerp8(endColor, (currentStep * 255) / (numSteps - 1));

  for (uint8_t i = startIndex; i <= endIndex; i++) {
    leds[i] = currentColor;
  }
}

void still() {
  Serial.println("Still");

  setSection1RGB(section1);
  setSection2RGB(section2);
  setRightEarRGB(rightEar);
  setSection3RGB(section3);
  setSection4RGB(section4);
  setLeftEarRGB(leftEar);
  setSection5RGB(section5);

  delay(1000);

  FastLED.show();
}

void setDefaults() {
  setSection1RGB(section1Default);
  setSection2RGB(section2Default);
  setRightEarRGB(rightEarDefault);
  setSection3RGB(section3Default);
  setSection4RGB(section4Default);
  setLeftEarRGB(leftEarDefault);
  setSection5RGB(section5Default);

  // set brightness levels
  // section 1
  for (int ledIndex = 0; ledIndex < 8; ledIndex++) {
    myleds[ledIndex].nscale8(128);
  }

  // section 2
  for (int ledIndex = 8; ledIndex < 16; ledIndex++) {
    myleds[ledIndex].nscale8(128);
  }

  // rightEar
  for (int ledIndex = 16; ledIndex < 18; ledIndex++) {
    myleds[ledIndex].nscale8(255);
  }

  // section 3
  for (int ledIndex = 18; ledIndex < 26; ledIndex++) {
    myleds[ledIndex].nscale8(128);
  }

  // section 4
  for (int ledIndex = 26; ledIndex < 34; ledIndex++) {
    myleds[ledIndex].nscale8(128);
  }

  // leftEar
  for (int ledIndex = 34; ledIndex < 36; ledIndex++) {
    myleds[ledIndex].nscale8(255);
  }

  // section 5
  for (int ledIndex = 36; ledIndex < 44; ledIndex++) {
    myleds[ledIndex].nscale8(128);
  }
}

void loop() {
  switch (mode) {
    case 0:
      cycleRight();
      break;
    case 1:
      cycleLeft();
      break;
    case 2:
      still();
      break;
    default:
      cycleRight();
      break;
  }

  FastLED.show();
  FastLED.delay(10);  // 10 ms delay for smooth animation

  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttClient.loop();
}
