#include "MAX30102_Processing.h"

MAX30102_Processing::MAX30102_Processing()
{
    // Constructor
}

boolean MAX30102_Processing::begin(TwoWire &wirePort, HardwareSerial &Serialx)
{
    _Serialx = &Serialx;

    if (!particleSensor.begin(wirePort, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
    {
        _Serialx->println(F("MAX30105 was not found. Please check wiring/power."));
        return false;
    }
    return true;
}

void MAX30102_Processing::mesure(){
    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
    //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < buffer_len ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    red_buffer[i] = particleSensor.getRed();
    ir_buffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    _Serialx->print(F("red="));
    _Serialx->print(red_buffer[i], DEC);
    _Serialx->print(F(", ir="));
    _Serialx->println(ir_buffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(ir_buffer, buffer_len, red_buffer, &spo2, &valid_spo2, &heartrate, &valid_heartrate);


    uint8_t rs_idx = 0;
    uint8_t rh_idx = 0;
  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (rh_idx < (buffer_len-1) || rs_idx < (buffer_len-1))
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      red_buffer[i - 25] = red_buffer[i];
      ir_buffer[i - 25] = ir_buffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data


      red_buffer[i] = particleSensor.getRed();
      ir_buffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      _Serialx->print(F("red="));
      _Serialx->print(red_buffer[i], DEC);
      _Serialx->print(F(", ir="));
      _Serialx->print(ir_buffer[i], DEC);

      _Serialx->print(F(", HR="));
      _Serialx->print(heartrate, DEC);

      _Serialx->print(F(", HRvalid="));
      _Serialx->print(valid_heartrate, DEC);

      _Serialx->print(F(", SPO2="));
      _Serialx->print(spo2, DEC);

      _Serialx->print(F(", SPO2Valid="));
      _Serialx->println(valid_spo2, DEC);

      // get 100 sample
        if (valid_spo2 && spo2 >= 90){
            raw_spo2[rs_idx] = spo2;
            if(rs_idx < (buffer_len - 1)) rs_idx++ ;
        }
        if(valid_heartrate){
            raw_heartrate[rh_idx] = heartrate;
            if(rh_idx < (buffer_len - 1)) rh_idx++;
        }
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(ir_buffer, buffer_len, red_buffer, &spo2, &valid_spo2, &heartrate, &valid_heartrate);
    
  }

    _Serialx->println("END");
     _Serialx->println("Raw Value");
  for(uint8_t i = 0; i < 100; ++i){
      _Serialx->printf("sample %d : %d,%d\n",i,raw_heartrate[i],raw_spo2[i]);
  }


}


void MAX30102_Processing::getProcessData(uint8_t &heartbeat, uint8_t &spo2){
  uint16_t sum_heartbeat, sum_spo2;
  for(uint8_t i = 0; i < buffer_len; ++i){
    sum_heartbeat += raw_heartrate[i];
    sum_spo2 += raw_spo2[i];
  }
  heartbeat = sum_heartbeat/100;
  spo2 = sum_spo2/100; 
}
