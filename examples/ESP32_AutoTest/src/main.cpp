#include <Arduino.h>
#include <UniversalMMWave.h>

// Pins for ESP32 DevKit V1
#define RX_PIN 16
#define TX_PIN 17

UniversalMMWave sensor(RX_PIN, TX_PIN);

void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    Serial.println("================================");
    Serial.println("Universal mmWave Driver Test");
    Serial.println("1. Detecting Baud Rate...");
    Serial.println("================================");
    
    // This handles Auto-Baud and Config
    sensor.begin();
    
    Serial.print("Baud Rate Detected: ");
    Serial.println(sensor.getDetectedBaud());
    
    Serial.println("Waiting for Data...");
}

void loop() {
    if (sensor.update()) {
        // Only print if there is a target
        if (sensor.hasTarget()) {
            MMWaveTarget t = sensor.getTarget();
            
            Serial.print("[ID:"); Serial.print(t.id); Serial.print("] ");
            Serial.print("Status: "); Serial.print(t.status);
            Serial.print(" | Dist: "); Serial.print(t.distance, 1); Serial.print("m");
            
            // Speed Logic
            float kmh = abs(t.speed * 3.6);
            if (kmh > 0.5) {
                Serial.print(" | Speed: "); Serial.print(kmh, 1); Serial.println(" km/h");
            } else {
                Serial.print(" | Energy: "); Serial.println((int)t.energy);
            }
            
            // Print Debug Info once per target (optional)
            // Serial.print(" [Fix: "); Serial.print(sensor.getCorruptionMode()); Serial.print("]");
        }
    }
}
