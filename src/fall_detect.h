#pragma once
#include "sensors.h"

void fallDetectBegin();
bool fallDetected(const SensorData& s);  // true เมื่อยืนยันล้ม