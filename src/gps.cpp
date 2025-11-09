#include "gps.h"
#include <config.h>
#include <TinyGPSPlus.h>

static TinyGPSPlus gps;
static HardwareSerial GPSSerial(1);
static GPSFix lastFix;
static uint32_t lastUpdateMs = 0;

void gpsBegin(){
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void gpsPoll(){
  while(GPSSerial.available()){
    char c=(char)GPSSerial.read();
    gps.encode(c);
    lastUpdateMs = millis();
  }
  if (gps.location.isUpdated()){
    lastFix.valid = gps.location.isValid();
    lastFix.lat   = gps.location.lat();
    lastFix.lon   = gps.location.lng();
    lastFix.sats  = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;
    lastFix.hdop  = gps.hdop.isValid() ? gps.hdop.hdop() : 99.9f;
    lastFix.age_ms= gps.location.age();
  } else {
    if (lastFix.age_ms < 0xFFFFFF00UL){
      uint32_t dt = (millis() - lastUpdateMs);
      lastFix.age_ms += dt;
      lastUpdateMs = millis();
    }
  }
}

static bool pass(const GPSFix& f){
  if (!f.valid) return false;
  if (f.sats < GPS_MIN_SATS) return false;
  if (f.age_ms > GPS_MAX_AGE_MS) return false;
  return true;
}
GPSFix gpsGetFix(){ GPSFix f=lastFix; f.valid = pass(lastFix); return f; }
GPSFix gpsGetRaw(){ return lastFix; }