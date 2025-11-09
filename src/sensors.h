#pragma once
#include <Arduino.h>

struct SensorData {
  float A;      // total acceleration (m/s^2)
  float roll;   // deg
  float pitch;  // deg
  bool  shock;  // latched event (true for SHOCK_LATCH_MS)
};

void sensorsBegin();
SensorData sensorsRead();