#include <Arduino.h>
#include "Wire.h"
#include "Network.h"
#include "MCP79411.h"
#include "LiteButton.h"
#include "MAX30102_Processing.h"

#define ALERT_INTERVAL 10 // minute (range 0 - 60)
#define MEASURE_GAP 1     // hour (range 1 - 3)

#define BUZZER_PIN 13

#define BUZZER_DURATION 5000 // ms

#define MEASURE_BUTTON1_PIN 16
#define ALERT_BUTTON2_PIN 14

#define I2C_SDA1 4
#define I2C_SCL1 5

// insert your wifi infomation here
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// insert your firebase infomation here
#define FIREBASE_PROJECT_ID ""
#define API_KEY ""
#define USER_EMAIL ""
#define USER_PASSWORD ""

// insert your line token here
#define LINE_TOKEN ""

LiteButton MeasureButton1(MEASURE_BUTTON1_PIN);
LiteButton AlertButton2(ALERT_BUTTON2_PIN);

MCP79411 RTC;
Network IOTNetwork;
MAX30102_Processing HealthSensor;

static Timestamp timestamp_arr[3];
static uint8_t timestamp_idx = 0;
static DMYstamp dmy;

static int freq = 2000;
static int channel = 0;
static int resolution = 8;

uint8_t getCloseAlertIdx(tmElements_t tm, Timestamp timestamp[3]);
bool canAlert(tmElements_t tm, Timestamp timestamp);
bool canMeasure(tmElements_t tm, Timestamp timestamp);
bool isGreaterEqualThan(tmElements_t tm, Timestamp timestamp);

void updateDMY(tmElements_t tm);

void lineShowTableDetail(String init_msg, uint8_t start_idx);
String getTimeTable(uint8_t start_idx);

void setup()
{
    Wire.begin();
    Wire1.begin(I2C_SDA1, I2C_SCL1);
    RTC.begin(Wire1);

    MeasureButton1.begin();
    AlertButton2.begin();

    ledcSetup(channel, freq, resolution);
    ledcAttachPin(BUZZER_PIN, channel);

    Serial.begin(115200);

    tmElements_t tm_set = {
        30,  //  Second
        29,  //  Minute
        13,  //  Hour
        Sat, //  Wday
        21,  //  Day
        5,   //  Month
        52   //  Year
    };

    while (!Serial)
        ;

    while (!HealthSensor.begin(Wire, Serial))
    {
        delay(2000);
    }
    Serial.println("\n............ Sensor setup complete ..........\n");
    IOTNetwork.setupSerialOutput(Serial);
    IOTNetwork.setupWifi(WIFI_SSID, WIFI_PASSWORD);
    IOTNetwork.setupFireStore(FIREBASE_PROJECT_ID, API_KEY, USER_EMAIL, USER_PASSWORD);
    IOTNetwork.setupLine(LINE_TOKEN);
    Serial.println("\n............ Network setup complete ..........\n");
    RTC.timeWrite(tm_set);
    Serial.println("\n............ RTC setup complete ..........\n");

    IOTNetwork.firebaseReadTimeStamp(timestamp_arr);
    tmElements_t tm = RTC.timeRead();
    timestamp_idx = getCloseAlertIdx(tm, timestamp_arr);
    RTC.printTimeFormat(Serial);

    //  Line Notify Setup
    lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดวันนี้\n", timestamp_idx);
    updateDMY(tm);
}

void loop()
{
    static bool patient_calling = false;
    static uint32_t rtc_last_millis = 0;
    static uint8_t rtc_last_hour = 0;
    static tmElements_t tm;
    static uint32_t last_buzzer_time = millis();

    if (millis() - rtc_last_millis >= 1000)
    {
        rtc_last_millis = millis();
        Timestamp ts = timestamp_arr[timestamp_idx];
        tm = RTC.timeRead();
        RTC.printTimeFormat(Serial);

        if (canAlert(tm, ts))
        {
            IOTNetwork.lineNotify("อย่าลืมวัดชีพจรและออกซิเจน");
        }
        if (!canMeasure(tm, ts) && isGreaterEqualThan(tm, ts))
        {

            IOTNetwork.lineNotify("คุณได้วัดเลยช่วงเวลาวัดก่อนหน้าแล้ว");
            if (timestamp_idx == 2)
            {
                timestamp_idx = 0;
                IOTNetwork.firebaseReadTimeStamp(timestamp_arr);
                lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดวันพรุ่งนี้\n", timestamp_idx);
            }
            else
            {
                timestamp_idx += 1;
                lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดถัดไปวันนี้\n", timestamp_idx);
            }
        }

        if (rtc_last_hour != tm.Hour)
        {
            if (tm.Hour == 0 && rtc_last_hour == 23)
            {
                updateDMY(tm);
            }
            rtc_last_hour = tm.Hour;
        }
    }

    IOTNetwork.loop();

    if (MeasureButton1.isPressed())
    {
        Timestamp ts = timestamp_arr[timestamp_idx];

        if (canMeasure(tm, ts))
        {
            uint8_t heartbeat, spo2;
            IOTNetwork.lineNotify("เริ่มการวัด");
            HealthSensor.mesure();
            HealthSensor.getProcessData(heartbeat, spo2);
            Serial.printf("heartbeat = %d, spo2 = %d\n",heartbeat, spo2);
            String msg = "การวัดเสร็จเสร็จสิ้นคุณมีค่า\n";
            msg += "อัตราการเต้นของหัวใจ :" + String(heartbeat) + "\n";
            msg += "ออกซิเจนในเลือด: " + String(spo2);
            IOTNetwork.lineNotify(msg);

            if (timestamp_idx == 2)
            {
                IOTNetwork.firebaseReadTimeStamp(timestamp_arr);
                lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดวันพรุ่งนี้\n", 0);
            }
            else
            {
                lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดถัดไปวันนี้\n", timestamp_idx + 1);
            }

            char sbuf[50];
            sprintf(sbuf, "%02d:%02d:%02d\n", tm.Hour, tm.Minute, tm.Second);
            IOTNetwork.firebaseUpdateLog(dmy, String(sbuf), timestamp_idx, heartbeat, spo2);
            timestamp_idx = (timestamp_idx + 1) % 3;
        }
        else
        {
            IOTNetwork.lineNotify("ยังไม่ถึงช่วงเวลาวัด");
            lineShowTableDetail("ตารางเวลาวัดชีพจรและออกซิเจนในเลือดวันนี้\n", timestamp_idx);
        }
    }

    if (AlertButton2.isPressed())
    {
        patient_calling = !patient_calling;
        IOTNetwork.firebaseAlert(patient_calling);
        if (patient_calling){
            IOTNetwork.lineNotify("เราทำการเรียกพยาบาลให้ท่านแล้ว โปรดรอสักครู่...");
        }
        else
        {
            IOTNetwork.lineNotify("เราทำการยกเลิกการเรียกพยาบาลให้ท่านแล้ว");
        }
    }

    
    if (patient_calling)
    {
        // uint8_t dutyCycle = patient_calling ? 200 : 0;
        if( millis() - last_buzzer_time <= BUZZER_DURATION){
            ledcWrite(channel, 200);
        }
        else{
            ledcWrite(channel, 0);
        }

    }
    else
    {
        last_buzzer_time = millis();
    }
}

uint8_t getCloseAlertIdx(tmElements_t tm, Timestamp timestamp[3])
{
    uint8_t min_val = abs(tm.Hour - timestamp[0].hour);
    uint8_t min_idx = 0;

    for (uint8_t i = 0; i < 3; ++i)
    {
        uint8_t hour_abs_diff = abs(tm.Hour - timestamp[i].hour);

        if (min_val > hour_abs_diff)
        {
            min_val = hour_abs_diff;
            min_idx = i;
        }
    }

    if (!canMeasure(tm, timestamp[min_idx]))
        min_idx = (min_idx + 1) % 3;
    return min_idx;
}

bool canAlert(tmElements_t tm, Timestamp timestamp)
{
    uint8_t hour_abs_diff = abs(tm.Hour - timestamp.hour);
    if (hour_abs_diff == 0 && tm.Second == timestamp.second)
    {
        return (abs(tm.Minute - timestamp.minute) == ALERT_INTERVAL || tm.Minute == timestamp.minute);
    }
    else if (hour_abs_diff == 1 && tm.Second == timestamp.second)
    {
        return (abs(tm.Minute - timestamp.minute) == (60 - ALERT_INTERVAL));
    }
    return false;
}

bool canMeasure(tmElements_t tm, Timestamp timestamp)
{
    int8_t hour_diff = tm.Hour - timestamp.hour;
    if (hour_diff == 0)
    {
        return true;
    }
    else if (hour_diff == MEASURE_GAP)
    {
        return (tm.Minute < timestamp.minute);
    }
    else if (hour_diff == -MEASURE_GAP)
    {
        return (tm.Minute >= timestamp.minute);
    }
    return false;
}

bool isGreaterEqualThan(tmElements_t tm, Timestamp timestamp)
{
    return tm.Hour >= timestamp.hour && tm.Minute >= timestamp.minute && tm.Second >= timestamp.second;
}

void updateDMY(tmElements_t tm)
{
    dmy.date = tm.Day;
    dmy.month = tm.Month;
    dmy.year = tm.Year + 1970;
}

void lineShowTableDetail(String init_msg, uint8_t start_idx)
{
    String msg = init_msg;
    msg += getTimeTable(start_idx);
    msg += String("คุณสามารถวัดค่าได้ภายในเวลาที่กำหนดโดยจะต้องไม่มากกว่าหรือน้อยกว่า 1 ชั่วโมง จากตารางเวลา");
    IOTNetwork.lineNotify(msg);
}

String getTimeTable(uint8_t start_idx)
{
    String msg;
    char sbuf[50];
    for (uint8_t i = start_idx; i < 3; ++i)
    {
        sprintf(sbuf, "เวลาที่%d : %02d:%02d:%02d\n", i + 1, timestamp_arr[i].hour, timestamp_arr[i].minute, timestamp_arr[i].second);
        msg += String(sbuf);
    }
    return msg;
}
