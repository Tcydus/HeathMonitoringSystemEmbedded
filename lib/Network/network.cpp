#include "Network.h"

Network::Network()
{
  // Constructor
}
void Network::setupSerialOutput(HardwareSerial &Serialx){
  _Serialx = &Serialx;
}

void Network::setupWifi(const char *ssid, const char *passphrase)
{
  _SSID = ssid;
  _PASSPHRASE = passphrase;
  WiFi.begin(ssid, passphrase);
  _Serialx->println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    _Serialx->print(".");
  }
  _Serialx->println("");
  _Serialx->print("Connected to WiFi network with IP Address: ");
  _Serialx->println(WiFi.localIP());
}

void Network::setupFireStore(const char* project_id,const char* api_key, const char* user_email, const char* user_password){

    _Serialx->printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    _PROJECT_ID = project_id;
    _API_KEY = api_key;
    _USER_EMAIL = user_email;
    _USER_PASSWORD = user_password;

    config.api_key = api_key;

    auth.user.email = user_email;
    auth.user.password = user_password;

    // config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);
}

void Network::setupLine(const char* line_token){
  _LINE_TOKEN = line_token;
}

void Network::loop()
{
  static uint32_t last_time;
  if (millis() - last_time >= LOOP_SPD)
  {
    last_time = millis();
    if (WiFi.status() != WL_CONNECTED)
    {
      _Serialx->println("---------Wifi disconnected---------");
      _Serialx->println("Try to connect");
      setupWifi(_SSID,_PASSPHRASE);
      setupFireStore(_PROJECT_ID, _API_KEY, _USER_EMAIL, _USER_PASSWORD);
      setupLine(_LINE_TOKEN);

    }
  }
}


void Network::firebaseReadTimeStamp(Timestamp timestamp[3]){
    String documentPath = "monitor/time_assignment";

    _Serialx->print("Get a document... ");

    if (Firebase.Firestore.getDocument(&fbdo, _PROJECT_ID, "", documentPath.c_str()))
    {
        _Serialx->print("OK\n\n");
        _Serialx->println(fbdo.payload());
    }
    else
    {
        _Serialx->println(fbdo.errorReason());
    }

    FirebaseJson payload;
    payload.setJsonData(fbdo.payload().c_str());
    FirebaseJsonData result;
    uint8_t i = 0;

    while (i < 3)
    {
        payload.get(result, "fields/timestamp" + String(i+1) + "/stringValue");
        if (result.success)
        {
            String buffer = result.to<String>();
            timestamp[i].hour = buffer.substring(0, 2).toInt();
            timestamp[i].minute = buffer.substring(3, 5).toInt();
            timestamp[i].second = buffer.substring(6, 8).toInt();
            i++;
        }
    }
}
void Network::firebaseUpdateLog(DMYstamp dmy, String timestamp, uint8_t time_index, double heartbeat, double spo2){
   char content_key[100];
    char documentPath[30];
    char time_index_str[6];
    FirebaseJson content;

    sprintf(documentPath, "sensor_log/%d-%02d-%02d", dmy.year, dmy.month, dmy.date);
    sprintf(time_index_str, "time%d", time_index+1);
    sprintf(content_key, "fields/%s/mapValue/fields/heartbeat_avg/integerValue", time_index_str);

    content.clear();
    content.set(content_key, heartbeat);

    sprintf(content_key, "fields/%s/mapValue/fields/spo2_avg/integerValue", time_index_str);
    content.set(content_key, spo2);

    sprintf(content_key, "fields/%s/mapValue/fields/timestamp/stringValue", time_index_str);
    content.set(content_key, timestamp);

    if (Firebase.Firestore.patchDocument(&fbdo, _PROJECT_ID, "", documentPath, content.raw(), time_index_str))
    {
        _Serialx->printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else if (Firebase.Firestore.createDocument(&fbdo, _PROJECT_ID, "", documentPath, content.raw()))
    {
        _Serialx->printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else
    {
        _Serialx->println(fbdo.errorReason());
    }
}


void Network::firebaseAlert(bool alert_status){
    FirebaseJson content;

    String documentPath = "monitor/status";

    content.clear();
    content.set("fields/alert_status/booleanValue", alert_status);

    if (Firebase.Firestore.patchDocument(&fbdo, _PROJECT_ID, "", documentPath.c_str(), content.raw(), "alert_status"))
    {
        _Serialx->printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else if (Firebase.Firestore.createDocument(&fbdo, _PROJECT_ID, "", documentPath.c_str(), content.raw()))
    {
        _Serialx->printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else
    {
        _Serialx->println(fbdo.errorReason());
    }
}

void Network::lineNotify(String msg_data, bool show_response) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("notify-api.line.me", 443)) {
    _Serialx->println("Connection failed");
    return;
  }

  String req = "";
  req += "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(_LINE_TOKEN) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP32\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + msg_data).length()) + "\r\n";
  req += "\r\n";
  req += "message=" + msg_data;
  
  client.print(req);
  delay(20);
  _Serialx->print("Sending -> ");
  _Serialx->println(msg_data);
  _Serialx->println("Sending Complete.");

  if (show_response) {
    _Serialx->println("-------------");
    _Serialx->println("<- Respond");
  }
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (show_response) _Serialx->println(line);
    if (line == "\r") break;
  }
  _Serialx->println("-------------");
}