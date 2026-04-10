#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <HardwareSerial.h>

/* =========================================================================
   HARDWARE PIN CONFIGURATION
   ========================================================================= */
#define LED_PIN          14   // PWM Output
#define RADAR_RX_PIN     17   // ESP32 RX (Connect to Sensor TX)
#define RADAR_TX_PIN     16   // ESP32 TX (Connect to Sensor RX)
#define I2C_SDA          21   // TSL2591 SDA
#define I2C_SCL          22   // TSL2591 SCL

/* =========================================================================
   SYSTEM TUNING PARAMETERS
   ========================================================================= */
const float LUX_DARK   = 5.0;     // Lux level for 100% Brightness
const float LUX_BRIGHT = 50.0;   // Lux level for 0% Brightness
const float SMOOTHING_FACTOR = 0.1; 
const unsigned long MOTION_HOLD_TIME = 5000; // 5 Seconds Hold
const unsigned long FADE_DURATION_MS = 510; // Time to fade from 0 to 100% (0.51 Seconds)

/* =========================================================================
   GLOBAL VARIABLES
   ========================================================================= */
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
HardwareSerial RadarSerial(1);

// System State
float smoothedLux = 0.0;          
unsigned long lastMotionTime = 0; 
bool motionActive = false;

// Brightness State (Float for smooth fading)
float currentBrightness = 0.0; // Where the LED IS right now
int targetBrightness = 0;      // Where the LED WANTS to be

// Radar Data
float lastSpeedMps = 0.0; 
String lastDirection = "None";
byte buffer[20];
int bufIndex = 0;
bool parsing = false;

// Timing
unsigned long lastLogicTime = 0; // For Sensors (200ms)
unsigned long lastFadeTime = 0;  // For Animation (20ms)

/* =========================================================================
   HELPER FUNCTIONS
   ========================================================================= */

// Map 0-255 input to 12-bit (0-4095) duty cycle
void ledcAnalogWrite(uint8_t pin, uint32_t value, uint32_t valueMax = 255) {
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);
  ledcWrite(pin, duty);
}

void printStatus(long timeSince, int ambientTarget, String source) {
  // Lux
  Serial.print("Lux: "); 
  Serial.print(smoothedLux, 1);
  Serial.print("\t");

  // Motion
  Serial.print("| Motion: ");
  if (motionActive) {
    Serial.print(lastDirection);
    Serial.print(" @ ");
    Serial.print(lastSpeedMps, 2); 
    Serial.print(" m/s (Hold: ");
    long remaining = (MOTION_HOLD_TIME - timeSince) / 1000;
    if (remaining < 0) remaining = 0;
    Serial.print(remaining);
    Serial.print("s)");
  } else {
    Serial.print("Scanning...");
  }
  Serial.print("\t");

  // Output
  Serial.print("| LED: ");
  Serial.print((int)currentBrightness); // Show ACTUAL brightness
  Serial.print("/");
  Serial.print(targetBrightness);      // Show TARGET brightness
  Serial.print(" [");
  Serial.print(source);
  Serial.println("]");
}

void readRadarData() {
  while (RadarSerial.available()) {
    byte inByte = RadarSerial.read();

    if (!parsing) {
      if (bufIndex == 0 && inByte == 0x55) buffer[bufIndex++] = inByte;
      else if (bufIndex == 1 && inByte == 0xA2) { buffer[bufIndex++] = inByte; parsing = true; }
      else bufIndex = 0;
      continue; 
    }

    if (parsing) {
      buffer[bufIndex++] = inByte;
      if (bufIndex >= 10) {
        if (buffer[2] == 0xC1) {
           byte state = buffer[7]; 
           if (state == 0x01 || state == 0x02) {
             uint16_t speedCmps = (buffer[5] << 8) | buffer[6];
             lastSpeedMps = speedCmps / 100.0; 
             lastDirection = (state == 0x01) ? "Leaving    " : "Approaching";
             lastMotionTime = millis(); 
           }
        }
        bufIndex = 0;
        parsing = false;
      }
    }
  }
}

/* =========================================================================
   SETUP
   ========================================================================= */
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- SMART STREETLIGHT SYSTEM INITIALISING ---");
  
  // 1. Give sensors time to power up cleanly
  Serial.println("Looking for sensors...");
  delay(2000);

  // 2. Setup PWM (12-bit)
  ledcAttach(LED_PIN, 5000, 12);

  // 3. Setup Radar
  RadarSerial.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  
  // 4. Radar Hardware Check
  bool radarFound = false;
  unsigned long startTime = millis();
  
  // Listen silently for up to 3 seconds
  while (millis() - startTime < 3000) {
    if (RadarSerial.available()) {
      radarFound = true;
      break; // We heard something, break out of the loop early
    }
  }

  // Print the result ONCE after the loop finishes
  if (radarFound) {
    Serial.println("Doppler radar connected");
    // Clear out any half-messages from the startup sequence
    while(RadarSerial.available()) RadarSerial.read();
  } else {
    Serial.println("ERROR: Doppler Radar not found!");
  }
  
  // 5. Setup Light Sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  if (tsl.begin()) {
    Serial.println("Light sensor connected");
  } else {
    Serial.println("ERROR: light sensor not found!");
    while (1);
  }
  
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);

  // Pre-load smoothing
  sensors_event_t event;
  tsl.getEvent(&event);
  if(event.light) smoothedLux = event.light;

  // Initialize Motion Timer to "Expired"
  lastMotionTime = -MOTION_HOLD_TIME; 

  Serial.println("--- SYSTEM ONLINE ---");
}

/* =========================================================================
   MAIN LOOP
   ========================================================================= */
void loop() {
  // 1. READ RADAR (Constant)
  readRadarData();

  // 2. SENSOR LOGIC (Runs every 200ms)
  if (millis() - lastLogicTime > 200) {
    lastLogicTime = millis();

    // --- A. READ LIGHT ---
    sensors_event_t event;
    tsl.getEvent(&event);
    float currentLux = event.light;
    
    if (currentLux >= 0 && currentLux < 50000) {
       smoothedLux = (smoothedLux * (1.0 - SMOOTHING_FACTOR)) + (currentLux * SMOOTHING_FACTOR);
    }

    // --- B. CHECK CONDITIONS ---
    // Is it Daytime?
    bool isDaytime = (smoothedLux >= LUX_BRIGHT);
    
    // Is there Motion?
    long timeSinceMotion = millis() - lastMotionTime;
    motionActive = (timeSinceMotion < MOTION_HOLD_TIME);

    // --- C. DECIDE TARGET ---
    String decisionSource = "";

    if (isDaytime) {
      targetBrightness = 0;
      decisionSource = "DAYTIME";
    } 
    else {
      if (motionActive) {
         targetBrightness = 255;
         decisionSource = "NIGHT (MOTION)";
      } else {
         targetBrightness = 0;
         decisionSource = "NIGHT (SAVING)";
         // Cleanup display
         lastSpeedMps = 0.0;
         lastDirection = "None";
      }
    }

    // --- D. DASHBOARD ---
    printStatus(timeSinceMotion, targetBrightness, decisionSource);
  }

  // 3. FADE ANIMATION (Runs every 20ms for smooth 50fps look)
  if (millis() - lastFadeTime > 20) {
    lastFadeTime = millis();

    // Calculate Step Size: Full Range (255) / (Duration / LoopTime)
    // E.g. 255 / (510ms / 20ms) = 10 steps per loop
    float step = 255.0 / (FADE_DURATION_MS / 20.0);
    
    // Move Current towards Target
    if (currentBrightness < targetBrightness) {
      currentBrightness += step;
      if (currentBrightness > targetBrightness) currentBrightness = targetBrightness;
    }
    else if (currentBrightness > targetBrightness) {
      currentBrightness -= step;
      if (currentBrightness < targetBrightness) currentBrightness = targetBrightness;
    }

    // Write to Hardware
    ledcAnalogWrite(LED_PIN, (int)currentBrightness);
  }
}