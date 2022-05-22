#ifndef MAX30102_PROCESSING_h
#define MAX30102_PROCESSING_h

#include "Arduino.h"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

static const uint8_t MAX_BRIGHTNESS = 255;

class MAX30102_Processing
{
    private:
        MAX30105 particleSensor;
        TwoWire *_i2cPort;
        HardwareSerial *_Serialx;
        
        int32_t buffer_len = 100;
        uint32_t ir_buffer[100];
        uint32_t red_buffer[100];

        int32_t spo2;
        int8_t valid_spo2; 
        int32_t heartrate;
        int8_t valid_heartrate;

        uint8_t raw_heartrate[100];
        uint8_t raw_spo2[100];
        uint8_t process_heartrate;
         uint8_t process_spo2;
        uint8_t raw_t[100];

        byte ledBrightness = 60; //Options: 0=Off to 255=50mA
        byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
        byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
        byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
        int pulseWidth = 411; //Options: 69, 118, 215, 411
        int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

        float Sx = 50.0f, Sx2 = 32.835f, Sx3 =  24.503f, Sx4 = 19.503f ; //
        float Sy,Sxy,Sx2y;
        
    public:
        MAX30102_Processing();
        boolean begin(TwoWire &wirePort, HardwareSerial &Serialx);
        void mesure();
        void getProcessData(uint8_t &heartbeat, uint8_t &spo2);
};

#endif
