#include "AQNetWork.h"

//https://api.seniverse.com/v3/weather/now.json?key=Sg68RU5VOEzrlcpsO&location=Baoding&language=en&unit=c
String weat_api = "/v3/weather/daily.json?key=";
const char *host = "api.seniverse.com";
const char *privateKey = "心知天气API Key";
const char *city = "城市名称,具体应查询心知天气官网的城市列表";
const char *language = "en";
char EOH[] = "\r\n\r\n"; //Header尾

const char* time_api = "http://api.m.taobao.com/rest/api3.do?api=mtop.common.getTimestamp";

DynamicJsonDocument doc(512);

const int TryCircle = 20; //Max Count of trying to do something
const int TimeOut = 20 * 1000;   //Timeout (ms)
uint8_t Connected = 0;

uint8_t Connect(String* ssid, String* password)
{
    Log(*ssid);
    Log(*password);
    WiFi.begin(ssid->c_str(), password->c_str());
    for (uint8_t i = 0; i < TryCircle; i++)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("WLAN_CONNECTED");
            return 1;
        }
        delay(500);
        Serial.print(".");
    }
    Connected = 0;
    return 0;
}

long long GetTimeStamp()
{
    HTTPClient client;
    client.begin(time_api);
    Log("Time--GET");
    int try_count = 1;
    while (client.GET() != 200)
    {
        Serial.print(".");
        delay(500);
        if (try_count++ > TryCircle)
            break;
    }
    String data = client.getString();
    Log(data);
    doc.clear();
    deserializeJson(doc, data);
    String time_stamp_str = doc["data"]["t"];
    Log(time_stamp_str);
    client.end();
    return atoll(time_stamp_str.c_str()) / 1000 + 8 * 60 * 60;
}

void GetWeather(Weather_data* w_data, String city)
{
    WiFiClient client;
    weat_api += privateKey;
    weat_api += "&location=";
    weat_api += city;
    weat_api += "&language=";
    weat_api += language;
    

    Log("Weather--GET");
    int try_count = 1;
    while (!client.connect(host, 80))
    {
        Serial.print(".");
        delay(500);
        if (try_count++ > TryCircle)
        {
            memset(w_data, 0, sizeof(Weather_data));
            return;
        }
    }
    client.print(String("GET ") + weat_api + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    bool ok = client.find(EOH);
    if (!ok)
    {
        Serial.println("No response or invalid response!");
    }
    String data_str="";
    data_str += client.readStringUntil('\n');
    Log(data_str);
    doc.clear();
    deserializeJson(doc, data_str);
    
    String weat_day = doc["results"][0]["daily"][0]["text_day"];
    String weat_night = doc["results"][0]["daily"][0]["text_night"];
    w_data->code_day = doc["results"][0]["daily"][0]["code_day"];
    w_data->code_night = doc["results"][0]["daily"][0]["code_day"];
    w_data->temp_high = doc["results"][0]["daily"][0]["high"];
    w_data->temp_low = doc["results"][0]["daily"][0]["low"];
    w_data->weat_day = weat_day;
    w_data->weat_night = weat_night;
    w_data->success = 1;
    return;
}
