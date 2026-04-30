#include <Wire.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_now.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2591.h"
#include "WS_DALI.h"
#include "BGT24LTR11.h"

/* =========================================================================
   HARDWARE PIN CONFIGURATION
   ========================================================================= */

#define I2C_SDA          23   // TSL2591 SDA
#define I2C_SCL          22   // TSL2591 SCL

/* =========================================================================
   SYSTEM TUNING PARAMETERS
   ========================================================================= */

const float LUX_BRIGHT = 50.0;  // The lux level for daytime (i.e. when the luminaire should be off)
const float ABSOLUTE_LUX_BRIGHT = 500.0;  // The lux level that overrides the radar to force daytime conditions
const float SMOOTHING_FACTOR = 0.1; // Smoothing factor so that lux levels don't change too rapidly
const unsigned long MOTION_HOLD_TIME = 5000;  // Time in ms that the luminaire stays at max brightness when motion is detected

/* =========================================================================
   GLOBAL VARIABLES
   ========================================================================= */

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

// Setup the Radar objects according to the ESP32-C6 architecture
#define COMSerial Serial0  
#define ShowSerial Serial  
BGT24LTR11<HardwareSerial> BGT;

// ESP-NOW Broadcast Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// System State
float smoothedLux = 0.0;          
unsigned long lastMotionTime = 0; 
bool motionActive = false;

// DALI Brightness State (0 to 100%)
int targetBrightness = 0;      
int lastTargetBrightness = -1; 

// Radar Data
float lastSpeedMps = 0.0; 
String lastDirection = "None";

// Timing
unsigned long lastLogicTime = 0; // For Sensors (200ms)

/* =========================================================================
   HELPER FUNCTIONS
   ========================================================================= */

void printStatus(long timeSince, int ambientTarget, String source) {
  // Construct a single payload string for both Serial and ESP-NOW
  String payload = "Lux: " + String(smoothedLux, 1) + "\t| Motion: ";

  if (motionActive) {
    long remaining = (MOTION_HOLD_TIME - timeSince) / 1000;
    if (remaining < 0) remaining = 0;
    payload += lastDirection + " @ " + String(lastSpeedMps, 2) + " m/s (Hold: " + String(remaining) + "s)\t";
  } else {
    payload += "Scanning...                          \t";
  }

  payload += "| DALI Target: " + String(ambientTarget) + "% [" + source + "]";

  // 1. Print locally
  ShowSerial.println(payload);

  // 2. Transmit via ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t *)payload.c_str(), payload.length());
}

/* =========================================================================
   SETUP
   ========================================================================= */

void setup() {
  ShowSerial.begin(115200);
  delay(2000);
  ShowSerial.println("\n--- SMART STREETLIGHT SYSTEM INITIALISING ---");

  // --- ESP-NOW INITIALIZATION ---
  WiFi.mode(WIFI_STA); // Set device as a Wi-Fi Station
  if (esp_now_init() != ESP_OK) {
    ShowSerial.println("ESP-NOW Init Failed");
  } else {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA; 
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      ShowSerial.println("Failed to add ESP-NOW peer");
    } else {
      ShowSerial.println("ESP-NOW initialized for broadcast.");
    }
  }

  // 1. Setup DALI Subsystem
  ShowSerial.println("Initializing DALI Bus...");
  DALI_Init();
  if (DALI_NUM > 0) {
    ShowSerial.println("DALI scan complete");
  }
  else {
    ShowSerial.println("DALI scan failed! Restarting...");
    setup();
  }
  ShowSerial.println("Looking for sensors...");
  delay(2000);

  // 2. Setup Doppler Radar using the Library
  COMSerial.begin(115200);
  BGT.init(COMSerial);
  
  // Set to Target Detection Mode (0). Loop until successful.
  int retries = 0;
  while (!BGT.setMode(0) && retries < 5) {
    delay(500);
    retries++;
  }
  if (retries < 5) {
    ShowSerial.println("Doppler radar connected");
  } else {
    ShowSerial.println("ERROR: doppler radar not found!");
    while (1);
  }

  // 3. Setup Light Sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  if (tsl.begin()) {
    ShowSerial.println("Light sensor connected");
  } else {
    ShowSerial.println("ERROR: light sensor not found!");
    while (1);
  }
  
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
  sensors_event_t event;
  tsl.getEvent(&event);
  if(event.light) smoothedLux = event.light;
  lastMotionTime = -MOTION_HOLD_TIME;

  ShowSerial.println("--- SYSTEM ONLINE ---");
}

/* =========================================================================
   MAIN LOOP
   ========================================================================= */

void loop() {
  // SENSOR LOGIC (Runs every 200ms)
  if (millis() - lastLogicTime > 200) {
    lastLogicTime = millis();

    // --- A. READ RADAR VIA LIBRARY ---
    uint16_t radarState = 0;
    uint16_t radarSpeed = 0;
    
    // getInfo() queries the sensor and returns 1 if valid data is received
    if (BGT.getInfo(&radarState, &radarSpeed)) {
        if (radarState == BGT24LTR11_TARGET_APPROACH || radarState == BGT24LTR11_TARGET_LEAVE) {
            lastSpeedMps = radarSpeed / 100.0; // Convert cm/s to m/s
            lastDirection = (radarState == BGT24LTR11_TARGET_LEAVE) ? "Leaving    " : "Approaching";
            lastMotionTime = millis(); 
        }
    }

    // --- B. READ LUX LEVEL ---
    sensors_event_t event;
    tsl.getEvent(&event);
    float currentLux = event.light;
    
    if (currentLux >= 0 && currentLux < 50000) {
       smoothedLux = (smoothedLux * (1.0 - SMOOTHING_FACTOR)) + (currentLux * SMOOTHING_FACTOR);
    }

    // --- C. CHECK CONDITIONS ---
    static bool isDaytime = false;
    // Hysteresis: Prevent the luminaire from blinding its own sensor
    if (targetBrightness == 0) {
        // If the light is off, trust the sensor normally
        isDaytime = (smoothedLux >= LUX_BRIGHT);
    } else {
        // If the light is ON, it is probably illuminating the sensor
        // Only force daytime if lux is very high
        if (smoothedLux >= ABSOLUTE_LUX_BRIGHT) {
            isDaytime = true;
        }
    }

    long timeSinceMotion = millis() - lastMotionTime;
    motionActive = (timeSinceMotion < MOTION_HOLD_TIME);

    // --- D. DECIDE TARGET ---
    String decisionSource = "";

    if (isDaytime) {
      targetBrightness = 0;
      decisionSource = "DAYTIME";
    } 
    else {
      if (motionActive) {
         targetBrightness = 100;
         decisionSource = "NIGHT (MOTION)";
      }
      else {
        if (targetBrightness > 0) {
          targetBrightness -= 2;  // Dim by 2% every 200ms (Takes 10 seconds to reach 0)

          if (targetBrightness < 0) {
            targetBrightness = 0; // Prevent brightess from dropping below 0
          }
          decisionSource = "NIGHT (FADING)";
        }
        else {
          // Once fade reaches 0, enter saving mode
          targetBrightness = 0;
          decisionSource = "NIGHT (SAVING)";
          lastSpeedMps = 0.0;
          lastDirection = "None";
        }
      }
    }

    // --- E. DALI TRANSMISSION (Only on state change) ---
    if (targetBrightness != lastTargetBrightness) {
      lastTargetBrightness = targetBrightness;
      
      if (DALI_NUM > 0) {
        for (int i = 0; i < DALI_NUM; i++) {
          Luminaire_Brightness(targetBrightness, DALI_Addr[i]);
        }
      }
    }

    // --- F. PRINT TO DASHBOARD ---
    printStatus(timeSinceMotion, targetBrightness, decisionSource);
  }
  delay(2);
}
