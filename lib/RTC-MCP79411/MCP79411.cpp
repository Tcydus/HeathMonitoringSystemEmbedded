#include "MCP79411.h"

MCP79411::MCP79411()
{
  // Constructor
}

boolean MCP79411::begin(TwoWire &wirePort, uint8_t i2caddr)
{

  _i2cPort = &wirePort; 
  _i2cPort->begin();
  _i2caddr = i2caddr;

  return true;
}

void MCP79411::timeWrite(tmElements_t &tm){

  _i2cPort->beginTransmission(I2C_RTC_ADDR);
  _i2cPort->write(SEC_REG);
  _i2cPort->write(0x00);
  _i2cPort->endTransmission();

  bool osc_running;
  do {
    _i2cPort->beginTransmission(I2C_RTC_ADDR);
    _i2cPort->write(WKDAY_REG);
    _i2cPort->endTransmission();
    _i2cPort->requestFrom(I2C_RTC_ADDR, 1);
    osc_running = _i2cPort->read() & _BV(OSCRUN);
  }
  while (osc_running);

  _i2cPort->beginTransmission(I2C_RTC_ADDR);
  _i2cPort->write(SEC_REG);
  _i2cPort->write(dec2bcd(tm.Second) | _BV(ST));
  _i2cPort->write(dec2bcd(tm.Minute));
  _i2cPort->write(dec2bcd(tm.Hour));
  _i2cPort->write(tm.Wday);
  _i2cPort->write(dec2bcd(tm.Day));
  _i2cPort->write(dec2bcd(tm.Month));
  _i2cPort->write(dec2bcd(tmYearToY2k(tm.Year)));
  _i2cPort->endTransmission();
}

tmElements_t MCP79411::timeRead(){

  _i2cPort->beginTransmission(I2C_RTC_ADDR);
  _i2cPort->write(SEC_REG);
  _i2cPort->endTransmission();

  _i2cPort->requestFrom(I2C_RTC_ADDR, 7);
  _tm.Second = bcd2dec(_i2cPort->read() & ~_BV(ST));
  _tm.Minute = bcd2dec(_i2cPort->read());
  _tm.Hour = bcd2dec(_i2cPort->read() & ~_BV(HR1224));
  _tm.Wday = _i2cPort->read() & ~(_BV(OSCRUN) | _BV(PWRFAIL) | _BV(VBATEN));
  _tm.Day = bcd2dec(_i2cPort->read());
  _tm.Month = bcd2dec(_i2cPort->read() & ~_BV(LP));
  _tm.Year = y2kYearToTm(bcd2dec(_i2cPort->read()));

  return _tm;
}


void MCP79411::printTimeFormat(HardwareSerial &Serialx){

  char sbuf[50];
  sprintf(sbuf, "%s. %02d/%02d/%02d %02d:%02d:%02d", WDAY_STR[_tm.Wday - 1], _tm.Day, 
                          _tm.Month, _tm.Year + 1970, _tm.Hour, _tm.Minute, _tm.Second);
                        
  if (_tm.Second != last_sec) {
    Serialx.println(sbuf);
    Serialx.println("////////////\n\n");
    last_sec = _tm.Second;
  }
}




inline uint8_t MCP79411::dec2bcd(uint8_t n)
{
  return n + 6 * (n / 10);
}
inline uint8_t MCP79411::bcd2dec(uint8_t n)
{
  return n - 6 * (n >> 4);
}