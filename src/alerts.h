#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// OLED
void oledBegin();
void oledReady();
void oledCrashWaiting();
void oledSending();
void oledFetchFailed(const String& err);
void oledHelpSent();
void scrollStart(const String (&lines)[8]);
void scrollStop();
void scrollUpdate();
bool scrollActive();

// Cancel
void cancelBegin();
bool cancelPressed();

// Buzzer
void buzzerBegin();
void buzzerBlinkStart();
void buzzerBlinkStop();
void buzzerBlinkUpdate();

//LED
void emerLedBegin();
void emerLedBlinkStart();
void emerLedBlinkStop();
void emerLedBlinkUpdate();

// OLED Ready + GPS status + WIFI
void oledReadyStatus(bool wifiReady, const IPAddress& ip, bool gpsReady, int sats);

// U8g2 instance (หากอยากใช้เอง)
extern U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2;