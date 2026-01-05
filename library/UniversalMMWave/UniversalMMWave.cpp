#include "UniversalMMWave.h"

// Sensor Frame Signatures
static const char* SIG_STD = "DFDMD"; 
static const char SIG_V25[] = {0x24, 0x04, 0x06, 0x04, 0x4D, 0x04, 0x00}; // Arduino V25 Signature
static const char SIG_V31[] = {0x44, 0x04, 0x06, 0x04, 0x0D, 0x04, 0x00}; // ESP32 V31 Signature

UniversalMMWave::UniversalMMWave(int rxPin, int txPin) 
    : _rx(rxPin), _tx(txPin), _bufIdx(0), _correctionMode(0), _lastRxTime(0) {
    
    // Initialize default target state
    _currentTarget.id = 0;
    _currentTarget.distance = 0.0f;
    _currentTarget.speed = 0.0f;
    _currentTarget.energy = 0.0f;
    _currentTarget.status = "Initializing";
    _currentTarget.moving = false;
}

long UniversalMMWave::detectBaudRate() {
    pinMode(_rx, INPUT);
    
    // Sample pulse widths to determine bit timing
    unsigned long minPulse = 100000;
    unsigned long startTime = millis();
    
    // Sampling window: 100ms
    while(millis() - startTime < 100) {
        unsigned long pulse = pulseIn(_rx, LOW, 20000); 
        if (pulse > 10 && pulse < minPulse) {
            minPulse = pulse;
        }
    }
    
    // Timing Analysis:
    // ~96us  -> 10416 Baud (Standard Sensor Rate)
    // ~104us -> 9600 Baud  (Fallback/Alternative)
    
    if (minPulse > 90 && minPulse < 100) return 10416;
    if (minPulse >= 100 && minPulse < 110) return 9600;
    
    return 10416; // Default to 10416 if indeterminate
}

void UniversalMMWave::begin() {
    // 1. Detect Baud Rate
    _baudRate = detectBaudRate();
    
    // 2. Initialize Serial Interface
    #ifdef ESP32
    _serial = &Serial2;
    Serial2.begin(_baudRate, SERIAL_8N1, _rx, _tx);
    #else
    // Fallback for non-ESP32 architectures
    // Note: SoftwareSerial implementation required for AVR
    #endif
    
    // 3. Configure Sensor Parameters
    // Sending configuration commands ensures consistent behavior
    delay(500);
    if (_serial) {
        _serial->print("sensorStop\r\n"); delay(50);
        _serial->print("setRange 0 25\r\n"); delay(50);
        _serial->print("setSensitivity 9 9\r\n"); delay(50);
        _serial->print("saveConfig\r\n"); delay(200);
        _serial->print("sensorStart\r\n"); delay(200);
    }
}

bool UniversalMMWave::update() {
    bool dataParsed = false;
    
    if (!_serial) return false;

    while (_serial->available()) {
        char c = _serial->read();
        
        // Safety Mask: Clear MSB to prevent ASCII extended char issues
        c = c & 0x7F;
        
        if (_bufIdx < MM_FRAME_BUFFER_SIZE - 1) {
            _recvBuf[_bufIdx++] = c;
        }
        _lastRxTime = millis();
    }
    
    // Frame Timeout: Process buffer if idle for >10ms
    if (_bufIdx > 0 && (millis() - _lastRxTime > 10)) {
        _recvBuf[_bufIdx] = 0; // Null Terminate
        
        // Parse Frame (Corrects bit-errors in place)
        parseFrame(_recvBuf); 
        
        // Persist corrected frame for external access
        strcpy(_lastFrame, _recvBuf); 
        
        dataParsed = true;
        _bufIdx = 0; // Reset Buffer
    }
    
    return dataParsed;
}

void UniversalMMWave::applyPayloadCorrection(char* str) {
    // V31 Correction: Inverted Bit 6 on specific bytes
    // Applies mainly to numeric characters '0' (0x30) -> 'p' (0x70)
    while (*str) {
        if (*str >= 0x60) *str = *str ^ 0x40; 
        if (*str == 0x4C) *str = ',';         
        str++;
    }
}

void UniversalMMWave::parseFrame(char* frame) {
    char* header = NULL;
    char* payloadStart = NULL;
    
    // Signature Detection Strategy
    
    // 1. Standard Header detection
    if ((header = strstr(frame, SIG_STD))) {
        _correctionMode = 0; 
        payloadStart = header; 
    }
    // 2. V31 Signature (ESP32 Specific Corruption)
    else if ((header = strstr(frame, SIG_V31))) {
        _correctionMode = 2; 
        applyPayloadCorrection(header + 6); 
        payloadStart = header + 6; 
    }
    // 3. V25 Signature (Arduino Specific Corruption)
    else if ((header = strstr(frame, SIG_V25))) {
        _correctionMode = 1; 
        payloadStart = header;
    }
    
    if (header && payloadStart) {
        // Tokenize Payload: "HEADER,ID,DIST,SPEED,ENERGY"
        char* parts[10];
        int idx = 0;
        char* ptr = payloadStart;
        
        parts[idx++] = ptr;
        while(*ptr && idx < 10) {
            if (*ptr == ',') {
                *ptr = 0;
                parts[idx++] = ptr + 1;
            }
            ptr++;
        }
        
        // Parse Numeric Data
        if (idx >= 5) {
            // Adjust index based on header stripping
            int baseIdx = 0;
            if (atoi(parts[1]) > 0 || strcmp(parts[1], "0") == 0) {
                baseIdx = 0;
            } else {
                baseIdx = 1; 
            }
            
            int id = atoi(parts[baseIdx + 1]);
            float d = atof(parts[baseIdx + 3]);
            float s = atof(parts[baseIdx + 4]);
            float e = atof(parts[baseIdx + 5]);
            
            if (id > 0) updateTargetState(id, d, s, e);
        }
    }
}

void UniversalMMWave::updateTargetState(int id, float d, float s, float e) {
    _currentTarget.id = id;
    _currentTarget.distance = d;
    _currentTarget.speed = s;
    _currentTarget.energy = e;
    
    // Status Logic
    if (abs(s) < 0.1) _currentTarget.status = "STATIONARY";
    else if (s > 0) _currentTarget.status = "RECEDING";
    else _currentTarget.status = "APPROACHING";
    
    _currentTarget.moving = (abs(s) >= 0.1);
}

// Accessors
bool UniversalMMWave::hasTarget() { return _currentTarget.id != 0; }
MMWaveTarget UniversalMMWave::getTarget() { return _currentTarget; }
const char* UniversalMMWave::getRawData() { return _lastFrame; }
long UniversalMMWave::getDetectedBaud() { return _baudRate; }
const char* UniversalMMWave::getCorrectionMode() {
    switch(_correctionMode) {
        case 2: return "V31 (Bit Correction)";
        case 1: return "V25 (Header Correction)";
        default: return "Standard";
    }
}
