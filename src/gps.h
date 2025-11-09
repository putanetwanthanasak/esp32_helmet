#pragma once
#include <Arduino.h>

struct GPSFix {
  bool  valid=false;
  double lat=0.0, lon=0.0;
  uint32_t age_ms=0;
  uint8_t sats=0;
  float   hdop=99.9f;
};

void gpsBegin();
void gpsPoll();      // non-blocking
GPSFix gpsGetFix();  // ผ่านเกณฑ์
GPSFix gpsGetRaw();  // ดิบล่าสุด