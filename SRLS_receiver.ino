#include <WiFi.h>
#include <esp_now.h>

// Callback function executed when data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  // Create a char buffer large enough to hold the incoming payload (max 250 bytes)
  char payloadString[251]; 
  
  // Copy the raw bytes into char array
  memcpy(payloadString, incomingData, len);
  
  // Null-terminate the array so the ESP32 knows it's a valid string
  payloadString[len] = '\0';
  
  // Print the received dashboard string to the Serial Monitor
  Serial.println(payloadString);
}

void setup() {
  // Use standard Serial for the Native USB on the receiver
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- SECONDARY TELEMETRY MONITOR ONLINE ---");
  Serial.println("Listening for SRLS packets...");

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the receive callback function
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // The ESP32 handles ESP-NOW reception completely asynchronously.
  // The main loop requires zero logic!
  delay(10);
}