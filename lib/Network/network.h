#ifndef NETWORK_h
#define NETWORK_h

#include "Arduino.h"

#include "Wifi.h"
#include <Firebase_ESP_Client.h>

typedef struct Timestamp
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} Timestamp;

typedef struct DMYstamp
{
    uint8_t date;
    uint8_t month;
    uint16_t year;
} DMYstamp;


static const uint16_t LOOP_SPD = 1000;

class Network
{
        private:
        const char *_SSID, *_PASSPHRASE;
        const char *_PROJECT_ID,*_API_KEY,*_USER_EMAIL,*_USER_PASSWORD;
        const char *_LINE_TOKEN;

        HardwareSerial *_Serialx;
      
        FirebaseData fbdo;
        FirebaseAuth auth;
        FirebaseConfig config;
    
        public:
        Network();
        void setupSerialOutput(HardwareSerial &Serialx);
        void setupWifi(const char* ssid, const char *passphrase);
        void setupFireStore(const char* project_id,const char* api_key, const char* user_email, const char* user_password);
        void setupLine(const char* line_token);

        
        void loop();
        void firebaseReadTimeStamp(Timestamp timestamp[3]);
        void firebaseUpdateLog(DMYstamp dmy, String timestamp, uint8_t time_index, double heartbeat, double spo2);
        void firebaseAlert(bool alert_status);
        void lineNotify(String msg_data, bool show_response = false);
};

#endif
