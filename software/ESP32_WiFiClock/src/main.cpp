//系统头
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <time.h>
#include <DHT.h>
#include <math.h>

//用户头
#include "AQNetWork.h"
#include "TextData.h"
#include "ScheduledTask.h"

//宏定义
#define TEXT_WIDTH(c, s) (6 * s * c)
#define TEXT_HEIGHT(c, s) (8 * s * c)
#define SECOND_DAY(h, m) (h * 3600 + m * 60)

#define BG_COLOR          ST7735_BLACK    //界面背景色
#define BLK_PIN           4               //背光控制引脚

//宏定义-配置
#define BLK_CTRL //自动背光控制

#ifdef BLK_CTRL

#define BLK_ON_SINCE_H    7               //07:00后开启背光
#define BLK_ON_SINCE_M    00              //
#define BLK_OFF_SINCE_H   23              //23:30后关闭背光
#define BLK_OFF_SINCE_M   30              //


#endif //BLK_CTRL

//用户瞎勾8写检查
#if (BLK_OFF_SINCE_H < 0 || BLK_OFF_SINCE_H >= 24 || BLK_ON_SINCE_H < 0 || BLK_ON_SINCE_H >= 24 ||BLK_OFF_SINCE_M < 0 || BLK_OFF_SINCE_M >= 60 ||BLK_ON_SINCE_M < 0 || BLK_ON_SINCE_M >= 60)
#error "Error Occured! Back Light On/Off Time isn't right"
#endif

enum  STATE{
  OFF = 0,
  ON
};
//全局变量
String ssid = "WIFI名称";
String pwd = "WIFI密码";

Weather_data weat_data;

hw_timer_t * timer = NULL;
time_t time_stamp;
volatile uint8_t time_changed = 0;
struct tm* time_s;
struct tm* time_old = {0};
uint8_t curblk_state = ON;

//全局对象
Adafruit_ST7735 tft = Adafruit_ST7735(/*CS*/15, /*AO(DC)*/14, /*SDA(MOSI)*/21, /*SCLK*/22, /*RST*/25);
DHT dht = DHT(23, DHT11);
ScheduledTask task(&time_stamp);

//函数声明
void ChangeTime();
void EveryMin();
void EveryHour();
void UpdateTime(time_t t_s);
void BLK(uint8_t state);
void AutoBLKCheck();

//函数实现
void setup(void) {
  //背光控制引脚初始化
  pinMode(4, OUTPUT);
  BLK(ON);
  //串口初始化
  Serial.begin(9600);
  timer = timerBegin(0, 80, true);                //初始化
  timerAttachInterrupt(timer, &ChangeTime, true);    //调用中断函数
  timerAlarmWrite(timer, 1000000, true);        //timerBegin的参数二 80位80MHZ，这里为1000000  意思为1秒
  timerAlarmEnable(timer);
  //任务初始化
  task.AddTask(EveryMin, 60);
  task.AddTask(EveryHour, 60 * 30);
  //TFT屏幕初始化
  tft.initR();
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 5);
  tft.print("Watting for connection...");
  
  if (Connect(&ssid, &pwd))
  {
    tft.setTextColor(ST7735_GREEN);
    tft.printf("\nSuccessfully connect to %s", ssid.c_str());
  }
  else
  {
    tft.setTextColor(ST7735_RED);
    tft.printf("\nFailed connect to %s", ssid.c_str());

    while (1)
    {

    }
  }
  dht.begin();
  time_stamp = (time_t)GetTimeStamp();
  task.SetTime(&time_stamp);
  
  UpdateTime(time_stamp);
  tft.fillScreen(BG_COLOR);

  tft.drawBitmap(0, 80, Icon_Sun, 24, 20, ST7735_YELLOW);
  tft.drawBitmap(0, 105, Icon_Moon, 24, 20, ST7735_WHITE);
  tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3, Icon_Temp, 16, 16, ST7735_RED);
  tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3 + 16 + 2, Icon_Humt, 16, 16, ST7735_CYAN);

  
  AutoBLKCheck();
  //EveryHour();
  //EveryMin();
}

void loop() {

  task.Check();
  if (time_changed)
  {
    time_changed = 0;
    UpdateTime(time_stamp);
    Serial.println(time_stamp);
    //Serial.printf("Temp:%.1f, Hum:%.1f\n", dht.readTemperature(), dht.readHumidity());
  }
}

void ChangeTime()
{
  time_stamp++;
  time_changed = 1;
}

void EveryMin()
{
  AutoBLKCheck();
  //刷新分钟
  tft.fillRect(0, 2, TEXT_WIDTH(5, 4), TEXT_HEIGHT(1, 4), BG_COLOR);
  tft.setTextSize(4);
  tft.setCursor(0, 2);
  tft.setTextColor(ST7735_ORANGE);
  tft.printf("%02d:%02d", time_s->tm_hour, time_s->tm_min);
  //读DHT11的数据
  //dht.read();
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  while (isnanf(hum) && isnanf(temp)) //上电读取可能会读到空数据,过滤空数据，一直读取直到有数据
  {
    Serial.println("Bad Value");
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    delay(500);
  }
  
  tft.fillRect(60 + 16, TEXT_HEIGHT(1, 4) + 3, 52, TEXT_HEIGHT(1, 2), BG_COLOR);
  tft.fillRect(60 + 16, TEXT_HEIGHT(1, 4) + 3 + 16 + 2, 52, TEXT_HEIGHT(1, 2), BG_COLOR);

  tft.setTextColor(ST7735_MAGENTA);
  tft.setTextSize(2);
  tft.setCursor(60 + 20, TEXT_HEIGHT(1, 4) + 3);
  tft.printf("%.1f", temp);
  tft.setCursor(60 + 20, TEXT_HEIGHT(1, 4) + 3 + 16 + 2);
  tft.printf("%.1f", hum);
  Serial.println("MIN");

}

void EveryHour()
{
  GetWeather(&weat_data);
  UpdateTime((time_t)GetTimeStamp());
  Serial.println("HOUR");
  //刷新日期，天气
  tft.fillRect(0, TEXT_HEIGHT(1, 4) + 22, TEXT_WIDTH(5, 2), TEXT_HEIGHT(1, 2), BG_COLOR);
  tft.setTextSize(2);
  tft.setCursor(0, TEXT_HEIGHT(1, 4) + 22);
  tft.setTextColor(ST7735_BLUE);
  tft.printf("%02d/%02d",time_s->tm_mon + 1, time_s->tm_mday);

  tft.fillRect(0, TEXT_HEIGHT(1, 4) + 1, 48, 16, BG_COLOR);
  tft.drawBitmap(0, TEXT_HEIGHT(1, 4) + 1, (uint8_t *)Char_XingQi[0], 16, 16, ST7735_GREEN, BG_COLOR);
  tft.drawBitmap(0 + 16, TEXT_HEIGHT(1, 4) + 1, (uint8_t *)Char_XingQi[1], 16, 16, ST7735_GREEN, BG_COLOR);
  tft.drawBitmap(0 + 16 + 16, TEXT_HEIGHT(1, 4) + 1, (uint8_t *)Char_Week[time_s->tm_wday], 16, 16, ST7735_GREEN, BG_COLOR);

  //获取可能失败，需要过滤失败的信息
  if (weat_data.success &&
      !weat_data.weat_day.isEmpty() &&
      !weat_data.weat_night.isEmpty() &&
      weat_data.weat_night != "null" &&
      weat_data.weat_day != "null")
  {

    tft.fillRect(0 + 30, 82, 98, TEXT_HEIGHT(1, 2), BG_COLOR);
    tft.fillRect(0 + 30, 107, 98, TEXT_HEIGHT(1, 2), BG_COLOR);
    tft.setTextSize(2);

    tft.setCursor(0 + 30, 82);
    tft.setTextColor(ST7735_YELLOW);
    tft.print(weat_data.weat_day);

    tft.setCursor(0 + 30, 107);
    tft.setTextColor(ST7735_WHITE);
    tft.print(weat_data.weat_night);
  }
}

void UpdateTime(time_t t_s)
{
  time_stamp = t_s;
  time_s = localtime(&time_stamp);
}

void BLK(uint8_t state)
{
  if (state)
    digitalWrite(BLK_PIN, HIGH);
  else
    digitalWrite(BLK_PIN, LOW);
}

void AutoBLKCheck()
{
#ifdef BLK_CTRL
  //自动背光
  int sec_day_now = SECOND_DAY(time_s->tm_hour, time_s->tm_min);
  
  if (SECOND_DAY(BLK_ON_SINCE_H, BLK_ON_SINCE_M) <= sec_day_now &&
      sec_day_now <= SECOND_DAY(BLK_OFF_SINCE_H, BLK_OFF_SINCE_M))
  {
    BLK(ON);
  }
  else
  {
    BLK(OFF);
  }
#endif //BLK_CTRL
}
