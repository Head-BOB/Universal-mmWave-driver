# Universal mmWave Driver for DFRobot C4001

A robust, platform-agnostic C++ driver for the DFRobot C4001 24GHz mmWave Human Detection Sensor.

This library addresses common hardware integration challenges, specifically signal corruption and baud rate drift encountered when interfacing 5V sensors with 3.3V microcontrollers (ESP32) or standard Arduino AVR architectures.

## Features

- **Automatic Baud Rate Detection**: Dynamically analyzes signal pulse width to determine the operating baud rate (Standard 9600 vs Drifting 10416), eliminating the need for hardcoded values.
- **Adaptive Error Correction**:
  - **V25 Mode**: Corrects header bit-shifting inherent to specific Arduino voltage dividers.
  - **V31 Mode**: Corrects payload bit-flipping observed on ESP32 hardware serial interfaces.
- **Smart Parsing**: abstracts raw hexadecimal data streams into actionable `Distance`, `Speed`, and `Energy` metrics.

## Installation

1. Copy the `UniversalMMWave` folder into your project's `lib` directory (PlatformIO) or `libraries` folder (Arduino IDE).
2. Include the header in your application.

## Usage

```cpp
#include <UniversalMMWave.h>

// Define RX/TX Pins
// Note: ESP32 requires HardwareSerial pins (e.g., RX2/TX2)
UniversalMMWave sensor(16, 17);

void setup() {
    Serial.begin(115200);
    
    // Initialize Driver
    // - Detects Baud Rate
    // - Configures Sensor (Range 25m, Max Sensitivity)
    sensor.begin();
    
    Serial.print("Detected Baud Rate: ");
    Serial.println(sensor.getDetectedBaud());
}

void loop() {
    // Process incoming data
    if (sensor.update()) {
        if (sensor.hasTarget()) {
            MMWaveTarget target = sensor.getTarget();
            
            Serial.print("ID: "); Serial.print(target.id);
            Serial.print(" | Distance: "); Serial.print(target.distance); Serial.print("m");
            Serial.print(" | Speed: "); Serial.print(target.speed); Serial.print("m/s");
            Serial.print(" | Status: "); Serial.println(target.status);
        }
    }
}
```

## Hardware Connection

| Sensor Pin | ESP32 Pin | Logic Level | Notes |
| :--- | :--- | :--- | :--- |
| **VCC** (Red) | 5V Source | 5V | **Do NOT** power from ESP32 3.3V. |
| **GND** (Black)| GND | Common | Common ground required. |
| **TX** (Green) | GPIO 16 (RX2) | 5V -> 3.3V | Voltage divider recommended (1k/2k). |
| **RX** (Yellow) | GPIO 17 (TX2) | 3.3V -> 5V | Direct connection typically safe. |

## Troubleshooting

- **No Data**: Ensure RX/TX are not swapped. Green wire is Sensor TX -> MCU RX.
- **Garbage Output**: The driver auto-corrects most corruption. If persistent, check your common ground connection.
- **Silence**: Reset power to the sensor to recalibrate the noise floor.

## License

This project is open-source. Please attribute appropriately.
