#include <ArduinoJson.h>
#include <FastLED.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "secrets.h"
#include "ColorTransition.h"


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

// this is where state of each LED is stored
CRGB myleds[NUM_LEDS];


// some simple color definitions
CRGB blue(0, 0, 255);
CRGB green(0, 255, 0);
CRGB lighterGreen(20, 255, 20);
CRGB yellow(255, 255, 0);
CRGB red(255, 0, 0);
CRGB orange(255, 165, 0);
CRGB purple(128, 0, 128);

struct LEDSection {
  int firstLEDIndex;
  int lastLEDIndex;
  float brightness;
};

const std::string mainSectionNamesAntiClock[] = {"section01", "section02", "section03", "section04", "section05"};

const float defaultMainSectionBrightness = 0.25;
const float defaultEarSectionBrightness = 1.0;

std::map<std::string, LEDSection> namedSections = {
  {"section01", {0, 7, defaultMainSectionBrightness}},
  {"section02", {8, 15, defaultMainSectionBrightness}},
  {"rightEar", {16, 17, defaultEarSectionBrightness}},
  {"section03", {18, 25, defaultMainSectionBrightness}},
  {"section04", {26, 33, defaultMainSectionBrightness}},
  {"leftEar", {34, 35, defaultEarSectionBrightness}},
  {"section05", {36, 43, defaultMainSectionBrightness}},
};

/*
 0 still
 1 cycle right
 2 cycle left
*/
uint8_t mode = 0;

std::map<std::string, CRGB> namedColors = {
  {"blue", blue},
  {"green", green},
  {"lighterGreen", lighterGreen},
  {"yellow", yellow},
  {"red", red},
  {"orange", orange},
  {"purple", purple},
};

// maintain state of sections to color name, separate from the real state in myleds
std::map<std::string, std::string> currentSectionColors = {};

// define the anti-clockwise color rotation order for the main sections (not ears)
const uint8_t mainSectionCount = 5;
const std::string mainSectionAntiClockColorOrder[] = {"blue", "green", "yellow", "red", "purple"};

// define the ear color rotation (L to R)
const std::string earColorOrder[] = {"orange", "lighterGreen"};

// the amount of ms that the color transition should take
const uint32_t transitionDuration = 500;

// how often the color changes in ms
const uint32_t transitionInterval = 10000;

uint32_t lastTransitionTime = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(myleds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  randomSeed(analogRead(0));
  
  // limit my draw to 1A at 5v of power draw
  FastLED.setMaxPowerInVoltsAndMilliamps(5,1000);

  setupWifi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  setStartupColors();
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

  // // Parse JSON data
  // StaticJsonDocument<256> doc;
  // DeserializationError error = deserializeJson(doc, payload_str);

  // if (error) {
  //   Serial.print(F("deserializeJson() failed: "));
  //   Serial.println(error.f_str());
  //   return;
  // }

  // if (strcmp(topic, "studio-logo/palette") == 0) {
  //   // Parse and set the global variables for palette
  //   JsonArray arr;

  //   arr = doc["section1"];
  //   section1 = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["section2"];
  //   section2 = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["rightEar"];
  //   rightEar = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["section3"];
  //   section3 = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["section4"];
  //   section4 = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["leftEar"];
  //   leftEar = CRGB(arr[0], arr[1], arr[2]);

  //   arr = doc["section5"];
  //   section5 = CRGB(arr[0], arr[1], arr[2]);

  //   // Save the color palette to NVS
  //   preferences.begin(appName, false);
  //   preferences.putBytes("section1", &section1, sizeof(CRGB));
  //   preferences.putBytes("section2", &section2, sizeof(CRGB));
  //   preferences.putBytes("rightEar", &rightEar, sizeof(CRGB));
  //   preferences.putBytes("section3", &section3, sizeof(CRGB));
  //   preferences.putBytes("section4", &section4, sizeof(CRGB));
  //   preferences.putBytes("leftEar", &leftEar, sizeof(CRGB));
  //   preferences.putBytes("section5", &section5, sizeof(CRGB));
  //   preferences.end();
  // } else if (strcmp(topic, "studio-logo/mode") == 0) {
  //   // Parse and set the global variable for mode
  //   mode = doc["mode"];

  //   // Save the mode to NVS
  //   preferences.begin(appName, false);
  //   preferences.putUChar("mode", mode);
  //   preferences.end();
  // } else {
  //   Serial.println(F("Unknown topic"));
  // }
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



void setSectionToColorByName(std::string sectionName, CRGB color) {
    LEDSection section = namedSections[sectionName];
    int firstLED = section.firstLEDIndex;
    int lastLED = section.lastLEDIndex;
    int count = lastLED - firstLED + 1;
    float brightness = section.brightness;

    // Scale down the R, G, and B values by the brightness factor
    CRGB adjustedColor = CRGB(round(color.r * brightness), round(color.g * brightness), round(color.b * brightness));

    // Set the color of the specified range of LEDs
    fill_solid(myleds + firstLED, count, adjustedColor);

    // Serial.print("Set section ");
    // Serial.print(sectionName.c_str());
    // Serial.print(" to color ");
    // printCRGB(adjustedColor);
    // Serial.println();
}




void setStartupColors() {
  // for each color name in mainSectionAntiClockColorOrder
  for(int i = 0; i < mainSectionCount; i++) {
    // get the color name
    std::string colorName = mainSectionAntiClockColorOrder[i];
    // get the color
    CRGB color = namedColors[colorName];
    
    std::string sectionName = mainSectionNamesAntiClock[i];
    // set the section to the color
    setSectionToColorByName(sectionName, color);

    // update currentSectionColors
    currentSectionColors[sectionName] = colorName;
  }

  // do the same for the ears
  for(int i = 0; i < 2; i++) {
    const std::string earSections[] = {"leftEar", "rightEar"};
    std::string colorName = earColorOrder[i];
    CRGB color = namedColors[colorName];
    std::string sectionName = earSections[i];
    setSectionToColorByName(sectionName, color);
    currentSectionColors[sectionName] = color;
  }
}

// struct ColorTransition {
//   std::string sectionName;
//   CRGB startColor;
//   CRGB endColor;
// };

void transitionColors(const std::vector<ColorTransition>& transitions, uint32_t duration) {
  uint32_t startTime = millis();
  uint32_t elapsedTime = 0;
  float ratio;

  // Serial.println("--");
  // Serial.println("--");
  // Serial.println("START");
  while (elapsedTime < duration) {
    elapsedTime = millis() - startTime;
    ratio = (float)elapsedTime / (float)duration;

    for (const auto& transition : transitions) {
      LEDSection section = namedSections[transition.sectionName];

      // if (transition.sectionName == "section02") {
      //   Serial.print("Transitioning section ");
      //   Serial.print(transition.sectionName.c_str());
      //   Serial.print(" from ");
      //   printCRGB(transition.startColor);
      //   Serial.print(" to ");
      //   printCRGB(transition.endColor);
      //   Serial.print(" at ratio ");
      //   Serial.println(ratio);
      // }


      CRGB currentColor;
      if (ratio < 1.00) {
        currentColor.r = transition.startColor.r + (transition.endColor.r - transition.startColor.r) * ratio;
        currentColor.g = transition.startColor.g + (transition.endColor.g - transition.startColor.g) * ratio;
        currentColor.b = transition.startColor.b + (transition.endColor.b - transition.startColor.b) * ratio;
      } else {
        currentColor = transition.endColor;
      }

      // if (transition.sectionName == "section02") {
      //   Serial.print("Current color: ");
      //   printCRGB(currentColor);
      //   Serial.println();
      // }

      // for (int i = section.firstLEDIndex; i <= section.lastLEDIndex; ++i) {
      //   // Set the currentColor for each LED in the specified section
      //   myleds[i] = currentColor;
      // }
      setSectionToColorByName(transition.sectionName, currentColor);
    }

    FastLED.show();
    FastLED.delay(5); // You can adjust the delay for smoother/faster transitions
  }
  // Serial.println("section02 is now");
  // for (int i = namedSections["section02"].firstLEDIndex; i <= namedSections["section02"].lastLEDIndex; ++i) {
  //   // Set the currentColor for each LED in the specified section
  //   printCRGB(myleds[i]);
  // }

  // Serial.println("--");
  // Serial.println("--");
  // Serial.println("END");

}

int findColorIndex(const std::string& colorName) {
  // Find the iterator pointing to the color name in the array
  const auto it = std::find(std::begin(mainSectionAntiClockColorOrder),
                            std::end(mainSectionAntiClockColorOrder),
                            colorName);

  // Check if the color was found
  if (it != std::end(mainSectionAntiClockColorOrder)) {
    // Return the index of the found color
    return std::distance(std::begin(mainSectionAntiClockColorOrder), it);
  } else {
    // Return -1 if the color was not found
    return -1;
  }
}

void cycleAntiClockwise() {

  // Create a vector to store the ColorTransition objects
  std::vector<ColorTransition> transitions;

  std::map<std::string, std::string> newColorsBySection = {};

  for (const auto& sectionName : mainSectionNamesAntiClock) {
    // create a ColorTransition for section1
    ColorTransition sectionTransition;
    sectionTransition.sectionName = sectionName;
    std::string startColorName = currentSectionColors[sectionName];
    // get the index of the current color in the mainSectionAntiClockColorOrder
    int currentIndex = findColorIndex(startColorName);
    // next index of color in mainSectionAntiClockColorOrder
    // int nextIndex = (currentIndex - 1) % mainSectionCount;
    int nextIndex = currentIndex == 0 ? mainSectionCount - 1 : currentIndex - 1;
    // get the next color name
    std::string nextColorName = mainSectionAntiClockColorOrder[nextIndex];

    // next color value
    CRGB startColor = namedColors[startColorName];
    CRGB nextColor = namedColors[nextColorName];
    sectionTransition.startColor = startColor;
    sectionTransition.endColor = nextColor;

    newColorsBySection[sectionName] = nextColorName;

    Serial.print("Transitioning section ");
    Serial.print(sectionTransition.sectionName.c_str());
    Serial.print(" from ");
    Serial.print(startColorName.c_str());
    Serial.print(" ");
    printCRGB(sectionTransition.startColor);
    Serial.print(", to ");
    Serial.print(nextColorName.c_str());
    Serial.print(" ");
    printCRGB(sectionTransition.endColor);
    Serial.println();

    // add the transition to the vector
    transitions.push_back(sectionTransition);
  }

  // now transition the colors
  transitionColors(transitions, transitionDuration);

  // after the transition, update the currentSectionColors from newColorsBySection
  currentSectionColors = newColorsBySection;
}

void printCRGB(const CRGB& color) {
  Serial.print("R: ");
  Serial.print(color.r);
  Serial.print(", G: ");
  Serial.print(color.g);
  Serial.print(", B: ");
  Serial.print(color.b);
}

void printAllLEDColors(const CRGB* leds, int numLEDs) {
  for (int i = 0; i < numLEDs; i++) {
    Serial.print("LED ");
    Serial.print(i);
    Serial.print(": ");
    printCRGB(leds[i]);
    Serial.println();
  }
}

void printCurrentSectionColors(const std::map<std::string, std::string>& currentSectionColors) {
  for (const auto& entry : currentSectionColors) {
    Serial.print("Section: ");
    Serial.print(entry.first.c_str());
    Serial.print(", Color: ");
    Serial.println(entry.second.c_str());
  }
}

void doTransition(int mode) {
  // Determine what to do based on the mode
  Serial.print("transitioning for mode: ");
  Serial.println(mode);
  switch (mode) {
    case 0:
      // Do something for mode 0
      setStartupColors();
      break;
    case 1:
      // Do something for mode 1
      break;
    // Add more cases for other modes as needed
    case 2:
      break;
    case 3:
      cycleAntiClockwise();
      break;
    default:
      break;
  }
}

int loopCount = 0;
void loop() {

 uint32_t currentTime = millis();

  if (currentTime - lastTransitionTime >= transitionInterval) {
    Serial.println("before transition, current colors:");
    printCurrentSectionColors(currentSectionColors);
    Serial.println("leds before transition:");
    printAllLEDColors(myleds, NUM_LEDS);
    Serial.println("");

    doTransition(3);

    Serial.println("");

    Serial.println("after transition , current colors:");
    printCurrentSectionColors(currentSectionColors);
    printAllLEDColors(myleds, NUM_LEDS);

    // printAllLEDColors(myleds, NUM_LEDS);
    lastTransitionTime = currentTime;
  }  else {
    Serial.println("skipped transition for debugging!");
    delay(10000);
  }

  FastLED.show();
  FastLED.delay(10);  // 10 ms delay for smooth animation

  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttClient.loop();
}
