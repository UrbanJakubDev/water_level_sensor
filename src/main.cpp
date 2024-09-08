#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "secrets.h"

// API Endpoint
const String apiEndpoint = "http://your-api-endpoint.com/data";

// Pin Definitions
const int trigPin = D5;
const int echoPin = D6;
const int analogPin = A0;  // For battery voltage reading

// Hysteresis thresholds
float levelHysteresis = 2.0;  // cm
float voltageHysteresis = 0.1;  // Volts

// Previous readings
float previousLevel = -1;
float previousVoltage = -1;

// WiFi Client
WiFiClient client;

// Deep sleep time (in microseconds)
const uint64_t sleepTimeInSeconds = 3600e6; // 1 hour (3600 seconds * 1,000,000)

void setup() {
  Serial.begin(115200);

  // Set pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi
  connectWiFi();

  // Measure water level and battery voltage
  float currentLevel = getDistance();
  float currentVoltage = readBatteryVoltage();

  // Only send data if there is significant change
  if (shouldSendData(currentLevel, previousLevel, levelHysteresis) || 
      shouldSendData(currentVoltage, previousVoltage, voltageHysteresis)) {
    Serial.println("Significant change detected, sending data...");
    sendDataToAPI(currentLevel, currentVoltage);

    // Update previous readings
    previousLevel = currentLevel;
    previousVoltage = currentVoltage;
  } else {
    Serial.println("No significant change detected, skipping data send.");
  }

  // Enter deep sleep
  Serial.println("Going to sleep...");
  ESP.deepSleep(sleepTimeInSeconds);
}

void loop() {
  // The ESP will never reach the loop due to deep sleep
}

float getDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Trigger the sensor by sending a HIGH pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pin
  long duration = pulseIn(echoPin, HIGH);

  // Calculate distance in cm
  float distance = (duration * 0.0343) / 2;
  return distance;
}

float readBatteryVoltage() {
  int analogValue = analogRead(analogPin);  // Read the analog value (0-1023)
  
  // Convert the analog value to actual battery voltage
  float voltage = analogValue * (4.2 / 1023.0) * 3;  // Adjust based on your resistor values
  return voltage;
}

bool shouldSendData(float currentValue, float previousValue, float hysteresis) {
  if (previousValue == -1) {
    // First reading, send data
    return true;
  }
  
  // Check if the change exceeds the hysteresis threshold
  return abs(currentValue - previousValue) >= hysteresis;
}

void sendDataToAPI(float level, float voltage) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(client, apiEndpoint);
    http.addHeader("Content-Type", "application/json");

    // Prepare JSON payload
    String payload = "{\"level\":" + String(level) + ",\"voltage\":" + String(voltage) + "}";

    // POST request to send data
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void connectWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected to Wi-Fi");
}
