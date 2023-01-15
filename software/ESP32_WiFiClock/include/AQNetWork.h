#ifndef _AQNETWORK_H_
#define _AQNETWORK_H_

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define DEBUG_ENABLE    0
#define Log(x)          (DEBUG_ENABLE ? Serial.println(x) : 0)
#define TEST_ENABLE     0

typedef struct WEATHER_DATA
{
    String weat_day;
    int code_day;
    String weat_night;
    int code_night;
    int temp_high;
    int temp_low;
    int success;
}Weather_data;

uint8_t Connect(String* ssid, String* password);
long long GetTimeStamp();
void GetWeather(Weather_data* w_data ,String city = "city");
uint8_t isConnected();

#endif //_AQNETWORK_H_
