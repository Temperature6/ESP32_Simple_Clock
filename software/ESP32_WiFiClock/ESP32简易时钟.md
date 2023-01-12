# ESP32简易时钟

### 简介

使用Arduino框架和ESP32开发的简易时钟

### 功能

⭐时间/星期/日期显示

⭐联网校时：每个小时自动联网校时一次（可设置）

⭐联网获取天气：获取指定城市的白天/夜晚的天气并显示

⭐温湿度显示：通过DHT11模块获取温湿度，并显示，每分钟刷新一次

⭐自动背光控制：可设置从何时开始打开背光，何时关闭背光，默认为7：00~23：30打开背光

⭐切换环境数据来源：点击按键可以切换板载LED的状态，长按1s可以切换环境状态数据的来源（DHT传感器或者Web）

### 参考

城市列表：[城市列表 | 心知天气文档 (seniverse.com)](https://docs.seniverse.com/product/data/city.html)

DHT11驱动：[adafruit/DHT-sensor-library: Arduino library for DHT11, DHT22, etc Temperature & Humidity Sensors (github.com)](https://github.com/adafruit/DHT-sensor-library)

ArduinoJson：[bblanchon/ArduinoJson: 📟 JSON library for Arduino and embedded C++. Simple and efficient. (github.com)](https://github.com/bblanchon/ArduinoJson)

ST7735 LCD屏幕[adafruit/Adafruit-ST7735-Library: This is a library for the Adafruit 1.8" SPI display http://www.adafruit.com/products/358 and http://www.adafruit.com/products/618 (github.com)](https://github.com/adafruit/Adafruit-ST7735-Library)

Adafruit Unified Sensor：[adafruit/Adafruit_Sensor: Common sensor library (github.com)](https://github.com/adafruit/Adafruit_Sensor)

### 版本

20230110-v1.0
20230112-v1.1:修复了天气文本内容过长会覆盖其他位置的Bug，增加了看门狗，防止死机，点击按键可以切换板载LED的状态，长按1s可以切换环境状态数据的来源（DHT传感器或者Web）

20230112-v1.2:修复了第二次请求数据失败的问题



