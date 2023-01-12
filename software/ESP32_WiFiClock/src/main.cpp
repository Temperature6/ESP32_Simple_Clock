//系统头
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <time.h>
#include <DHT.h>
#include <math.h>
#include <Preferences.h>
#include <string>

//用户头
#include "AQNetWork.h"
#include "TextData.h"
#include "ScheduledTask.h"

//宏定义
#define TEXT_WIDTH(c, s) (6 * s * c)
#define TEXT_HEIGHT(c, s) (8 * s * c)
#define SECOND_DAY(h, m) (h * 3600 + m * 60)

#define BG_COLOR                ST7735_BLACK    //界面背景色
#define BLK_PIN                 4               //背光控制引脚

#define PREF_SPACE              "ESP_WiFiClock" //Preference命名空间
#define PREF_ONBOARD_LED_STATE  "onBoardLED"    //板载LED状态
#define PREF_DATA_SOURCE        "dataSource"    //环境数据来源

//宏定义-配置
#define BLK_CTRL //自动背光控制

#ifdef BLK_CTRL

#define BLK_ON_SINCE_H          7               //07:00后开启背光
#define BLK_ON_SINCE_M          00              //
#define BLK_OFF_SINCE_H         23              //23:30后关闭背光
#define BLK_OFF_SINCE_M         30              //


#endif //BLK_CTRL

//用户瞎勾8写检查
#if (BLK_OFF_SINCE_H < 0 || BLK_OFF_SINCE_H >= 24 || BLK_ON_SINCE_H < 0 || BLK_ON_SINCE_H >= 24 ||BLK_OFF_SINCE_M < 0 || BLK_OFF_SINCE_M >= 60 ||BLK_ON_SINCE_M < 0 || BLK_ON_SINCE_M >= 60)
#error "Error Occured! Back Light On/Off Time isn't right"
#endif

enum STATE {
  OFF = 0,
  ON,
  DHT_DATA,
  WEB_DATA,
};
//全局变量
String ssid = "ssid";
String pwd = "pwd";

Weather_data weat_data;             //天气数据
Preferences pref;                   //存储数据
uint8_t temp_show = WEB_DATA;       //温度来源于 DHT_DATA~DHT11温湿度传感器 WEB_DATA~来源于心知天气

hw_timer_t * timer = NULL;          //主时钟
time_t time_stamp;                  //当前时间戳(s)
volatile uint8_t time_changed = 0;  //时间改变标志
struct tm* time_s;                  //时间格式结构体

uint8_t curblk_state = ON;          //当前背光状态
uint8_t onBoardLED = 0;             //板载LED状态

const int wdtTimeout = 30;          //看门狗溢出时间(s)
hw_timer_t *wdtTimer = NULL;        //看门狗定时器

//全局对象
Adafruit_ST7735 tft = Adafruit_ST7735(/*CS*/15, /*AO(DC)*/14, /*SDA(MOSI)*/21, /*SCLK*/22, /*RST*/25);
DHT dht = DHT(23, DHT11);
ScheduledTask task(&time_stamp);

//函数声明
String strCut(std::string str, uint16_t beg_index, uint16_t end_index);
void ChangeTime();
void EveryMin();
void EveryHour();
void UpdateTime(time_t t_s);
void BLK(uint8_t state);
void AutoBLKCheck();
void feedDog();
void ARDUINO_ISR_ATTR resetModule();
void printEnvpIcon();
void printEnvpData();
void switchDataSource();

//函数实现
void setup(void) {
  //背光控制引脚初始化
  pinMode(4, OUTPUT);
  //按键
  pinMode(18, INPUT_PULLDOWN);
  pinMode(2, OUTPUT);
  //读取配置
  pref.begin(PREF_SPACE);
  onBoardLED = pref.getInt(PREF_ONBOARD_LED_STATE, 0);
  temp_show = pref.getInt(PREF_DATA_SOURCE, DHT_DATA);

  digitalWrite(2, onBoardLED);
  //先开启背光
  BLK(ON);
  //串口初始化
  Serial.begin(9600);
  timer = timerBegin(0, 80, true);                  //初始化
  timerAttachInterrupt(timer, &ChangeTime, true);   //调用中断函数
  timerAlarmWrite(timer, 1000000, true);            //timerBegin的参数二 80位80MHZ，这里为1000000  意思为1秒
  timerAlarmEnable(timer);
  //任务初始化
  task.AddTask(EveryMin, 60);
  task.AddTask(EveryHour, 60 * 30);
  //看门狗
  wdtTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(wdtTimer, &resetModule, true); //attach callback
  timerAlarmWrite(wdtTimer, wdtTimeout * 1e6, false); //set time in us
  timerAlarmEnable(wdtTimer); 
  //TFT屏幕初始化
  tft.initR();
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 5);
  tft.print("Connecting......");
  
  if (Connect(&ssid, &pwd))
  {
    tft.setTextColor(ST7735_GREEN);
    tft.printf("\nSuccessfully connect to\n%s", ssid.c_str());
  }
  else
  {
    tft.setTextColor(ST7735_RED);
    tft.printf("\nFailed connect to\n%s", ssid.c_str());

    timerAlarmDisable(wdtTimer); //关掉看门狗
    while (1)
    {

    }
  }
  dht.begin();
  time_stamp = (time_t)GetTimeStamp();
  task.SetTime(&time_stamp);
  
  UpdateTime(time_stamp);
  tft.fillScreen(BG_COLOR);
  
  //打印图标
  printEnvpIcon();
  tft.drawBitmap(0, 80, Icon_Sun, 24, 20, ST7735_YELLOW);
  tft.drawBitmap(0, 105, Icon_Moon, 24, 20, ST7735_WHITE);


  //检查背光
  AutoBLKCheck();
}

void loop() {

  task.Check();
  if (time_changed)
  {
    time_changed = 0;
    UpdateTime(time_stamp);
    feedDog();
    //Log(time_stamp);
  }
  if (digitalRead(18) == HIGH)
  {
    int pressed = 0;
    while (digitalRead(18) == HIGH)
    {
      //Log("STAY PRESS");
      delay(50);
      pressed += 50;

      if (pressed >= 1000)
      {
        break;
      }
    }
    
    if (pressed < 1000)
    {
      onBoardLED = !onBoardLED;
      pref.putInt(PREF_ONBOARD_LED_STATE, onBoardLED);
      digitalWrite(2, onBoardLED);
    }
    else if (pressed >= 1000)
    {
      switchDataSource();
      while (digitalRead(18) == HIGH);
    }
    
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
  
  //刷新来自DHT11传感器的数据
  if (temp_show == DHT_DATA)
    printEnvpData();

}

void EveryHour()
{
  memset(&weat_data, 0, sizeof(Weather_data));
  GetWeather(&weat_data);
  UpdateTime((time_t)GetTimeStamp());

  Log("HOUR");
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

  if (weat_data.success &&
      !weat_data.weat_day.isEmpty() &&
      !weat_data.weat_night.isEmpty() &&
      weat_data.weat_night != "null" &&
      weat_data.weat_day != "null")
  {

    tft.fillRect(0 + 30, 82, 98, TEXT_HEIGHT(1, 2), BG_COLOR);
    tft.fillRect(0 + 30, 107, 98, TEXT_HEIGHT(1, 2), BG_COLOR);
    
    
    //刷新天气
    if (weat_data.weat_day.length() > 8)
    {
      tft.setTextSize(1);
      tft.setCursor(0 + 30, 87);
      tft.setTextColor(ST7735_YELLOW);
      tft.print(weat_data.weat_day);
    }
    else
    {
      tft.setTextSize(2);
      tft.setCursor(0 + 30, 82);
      tft.setTextColor(ST7735_YELLOW);
      tft.print(weat_data.weat_day);
    }

    if (weat_data.weat_night.length() > 8)
    {
      tft.setTextSize(1);
      tft.setCursor(0 + 30, 112);
      tft.setTextColor(ST7735_WHITE);
      tft.print(weat_data.weat_night);
    }
    else
    {
      tft.setTextSize(2);
      tft.setCursor(0 + 30, 109);
      tft.setTextColor(ST7735_WHITE);
      tft.print(weat_data.weat_night);
    }

    tft.setTextSize(2);

    //刷新来自web的高低温度
    if (temp_show == WEB_DATA)
      printEnvpData();
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

void printEnvpIcon()
{
  tft.fillRect(60, TEXT_HEIGHT(1, 4) + 3, 16, 16 + 16 + 2, BG_COLOR);
  
  if (temp_show == DHT_DATA)
  {
    tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3, Icon_Temp, 16, 16, ST7735_RED);
    tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3 + 16 + 2, Icon_Humt, 16, 16, ST7735_CYAN);
  }
  else if (temp_show == WEB_DATA)
  {
    tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3, Icon_Up, 16, 16, ST7735_RED);
    tft.drawBitmap(60, TEXT_HEIGHT(1, 4) + 3 + 16 + 2, Icon_Down, 16, 16, ST7735_GREEN);
  }
  
  
}

void printEnvpData()
{
  //清空
  tft.fillRect(60 + 16, TEXT_HEIGHT(1, 4) + 3, 52, TEXT_HEIGHT(1, 2), BG_COLOR);
  tft.fillRect(60 + 16, TEXT_HEIGHT(1, 4) + 3 + 16 + 2, 52, TEXT_HEIGHT(1, 2), BG_COLOR);
  //数据预处理
  float hum = 0.0f, temp = 0.0f;

  if (temp_show == DHT_DATA)
  {
    hum = dht.readHumidity();
    temp = dht.readTemperature();

    while (isnanf(hum) && isnanf(temp)) //上电读取可能会读到空数据
    {
      Log("Bad Value");
      hum = dht.readHumidity();
      temp = dht.readTemperature();
      delay(500);
    }
  }
  
  Log("-------");
  Log((int)weat_data.temp_high);
  Log((int)weat_data.temp_low);
  Log("-------");
  
  String d1 = strCut(std::to_string(temp_show == DHT_DATA ? dht.readTemperature() : (int)weat_data.temp_high), 0, 3);
  String d2 = strCut(std::to_string(temp_show == DHT_DATA ? dht.readHumidity() : (int)weat_data.temp_low), 0, 3);

  tft.setTextColor(ST7735_MAGENTA);
  tft.setTextSize(2);

  tft.setCursor(60 + 20, TEXT_HEIGHT(1, 4) + 3);
  tft.setTextColor(temp_show == DHT_DATA ? ST7735_RED : ST7735_RED);
  tft.print(d1);
  tft.setCursor(60 + 20, TEXT_HEIGHT(1, 4) + 3 + 16 + 2);
  tft.setTextColor(temp_show == DHT_DATA ? ST7735_CYAN : ST7735_GREEN);
  tft.print(d2);
}

String strCut(std::string str, uint16_t beg_index, uint16_t end_index)
{
  String ret = "";

  for (int i = beg_index; i <= (str.length() > end_index ? end_index : str.length()); i++)
    ret += str[i];
  return ret;
}

void switchDataSource()
{
  temp_show = temp_show == DHT_DATA ? WEB_DATA : DHT_DATA;
  pref.putInt(PREF_DATA_SOURCE, temp_show);

  printEnvpIcon();
  printEnvpData();
  
}

void feedDog()
{
  timerWrite(wdtTimer, 0); //reset timer (feed watchdog)
}

void ARDUINO_ISR_ATTR resetModule()
{
  Log("Reset");
  esp_restart();
}
