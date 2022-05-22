#ifndef MCP79411_h
#define MCP79411_h

#include "Arduino.h"
#include <Wire.h>


#define I2C_RTC_ADDR 0x6F

#define SEC_REG 0x00
#define WKDAY_REG 0x03
#define ST 7
#define HR1224 6
#define OSCRUN 5
#define PWRFAIL 4
#define VBATEN 3
#define LP 5


#define tmYearToY2k(Y) ((Y)-30)   // change offset to 2000
#define y2kYearToTm(Y) ((Y) + 30) // change offset to 1970

enum week_day {Sun = 1, Mon, Tues, Wed, Thur, Fri, Sat};

typedef struct
{
        uint8_t Second;
        uint8_t Minute;
        uint8_t Hour;
        uint8_t Wday; // day of week, sunday is day 1
        uint8_t Day;
        uint8_t Month;
        uint8_t Year; // offset from 1970;
} tmElements_t;

class MCP79411
{
        private:
        uint8_t last_sec;
        const char WDAY_STR[7][5] = {"Sun","Mon","Tues","Wed","Thur","Fri","Sat"};

        TwoWire *_i2cPort;
        uint8_t _i2caddr;

        tmElements_t _tm;

        inline uint8_t dec2bcd(uint8_t n);
        inline uint8_t bcd2dec(uint8_t n);
    
        public:
        MCP79411();
        boolean begin(TwoWire &wirePort = Wire, uint8_t i2caddr = I2C_RTC_ADDR);
        void timeWrite(tmElements_t &tm);
        tmElements_t timeRead();
        void printTimeFormat(HardwareSerial &Serialx);
};

#endif
