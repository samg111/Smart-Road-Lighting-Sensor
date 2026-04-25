#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "WS_DALI.h"

// The name and password of the WiFi access point
#define APSSID       "ESP32-C6-DALI"
#define APPSK        "waveshare"
extern bool DALI_Loop;
void setBrightness(int sliderId, int value);// 设置亮度的函数（需要实现具体逻辑）
void handleSwitch(uint8_t ledNumber);

void handleRoot();
void handleSetSlider();
void handleRGBOn();
void handleRGBOff();

void WIFI_Init();
void WIFI_Loop();



