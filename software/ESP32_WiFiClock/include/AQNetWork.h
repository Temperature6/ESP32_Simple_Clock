#ifndef _AQNETWORK_H_
#define _AQNETWORK_H_

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define DEBUG_ENABLE 1
#define Log(x)  (DEBUG_ENABLE == 1 ? Serial.println(x) : 0)

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
//ע����Ĭ�ϲ���String city = "Baoding"�и���Ϊ���س������ƣ������������Ӧ�ο���֪���������ĳ����б�
void GetWeather(Weather_data* w_data ,String city = "����");

#endif //_AQNETWORK_H_
