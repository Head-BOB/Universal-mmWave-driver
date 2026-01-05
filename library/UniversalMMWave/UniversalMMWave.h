#ifndef UNIVERSAL_MMWAVE_H
#define UNIVERSAL_MMWAVE_H

#include <Arduino.h>

#define MM_FRAME_BUFFER_SIZE 128

struct MMWaveTarget {
    int id;           
    float distance;   
    float speed;      
    float energy;     
    bool moving;      
    const char* status; 
};

class UniversalMMWave {
public:
    UniversalMMWave(int rxPin, int txPin);
    
    /**
     * @brief Initializes the driver, detects baud rate, and configures the sensor.
     * Use this in setup().
     */
    void begin();

    /**
     * @brief Reads serial data and parses frames.
     * @return true if a valid target frame was parsed.
     */
    bool update();

    /**
     * @brief Checks if a target is currently detected.
     */
    bool hasTarget();

    /**
     * @brief Returns the current target data.
     */
    MMWaveTarget getTarget();

    /**
     * @brief Returns the automatically detected baud rate.
     */
    long getDetectedBaud();

    /**
     * @brief Returns the active error correction mode string.
     */
    const char* getCorrectionMode();

    /**
     * @brief Returns the raw (error-corrected) data string for custom parsing.
     */
    const char* getRawData();

private:
    int _rx, _tx;
    HardwareSerial* _serial; 
    long _baudRate;
    
    char _recvBuf[MM_FRAME_BUFFER_SIZE];
    char _lastFrame[MM_FRAME_BUFFER_SIZE]; // Persist last frame
    int _bufIdx;
    unsigned long _lastRxTime;
    
    int _correctionMode; 
    MMWaveTarget _currentTarget;
    
    long detectBaudRate();
    void parseFrame(char* frame);
    void applyPayloadCorrection(char* frame);
    void updateTargetState(int id, float d, float s, float e);
};

#endif
