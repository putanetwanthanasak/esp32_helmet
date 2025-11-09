#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "config.h"
#include "sensors.h"
#include "fall_detect.h"
#include "alerts.h"
#include "crash_seq.h"
#include "gps.h"

// Firebase objects (ใช้ใน crash_seq.cpp)
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig fbConfig;

static void setupWiFi(){
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-t<12000){ delay(200); Serial.print("."); }
  Serial.println(WiFi.status()==WL_CONNECTED ? " OK" : " FAIL");
}
static void setupFirebase(){
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&fbConfig, &auth);
  Firebase.reconnectWiFi(true);
}

void setup(){
  Serial.begin(115200);

  // UI
  oledBegin();  oledReady();
  buzzerBegin(); cancelBegin();

  // Sensors & Logic
  sensorsBegin();
  fallDetectBegin();

  // GPS
  gpsBegin();

  // Network
  setupWiFi();
  setupFirebase();

  // Crash orchestrator
  crashSeqBegin();

  Serial.println("System Ready");
}

void loop(){
  SensorData s = sensorsRead();
  bool crashed = fallDetected(s);

  if (DEBUG_SERIAL){
    static unsigned long last=0;
    if (millis()-last > DEBUG_IMU_PERIOD_MS){
      last=millis();
      Serial.print("A:"); Serial.print(s.A,1);
      Serial.print(" R:"); Serial.print(s.roll,0);
      Serial.print(" P:"); Serial.print(s.pitch,0);
      Serial.print(" shock="); Serial.print(s.shock);
      Serial.print("  crash="); Serial.println(crashed?"YES":"no");
    }
  }

  if (crashed && !crashActive()){
    crashSeqStart();    // เริ่ม A: CRASH! (เสียง+จอ) → 30s → B: ส่ง LINE + scroll
  }

  crashSeqUpdate();     // ดูแลเสียง/จอ/GPS/LINE/Cancel แบบ non-blocking

  delay(10);
}