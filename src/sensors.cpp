#include "sensors.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>
#include <config.h>

static Adafruit_MPU6050 mpu;
static unsigned long lastMs = 0;
static float g_roll = 0, g_pitch = 0;

static bool readShock() {
  static int lastRaw=-1, debounced=-1;
  static unsigned long lastEdge=0, lastShock=0;
  unsigned long now=millis();
  int raw = digitalRead(SHOCK_PIN);

  if (raw != lastRaw) { lastRaw=raw; lastEdge=now; }
  if (debounced==-1) {
    if (now-lastEdge >= SHOCK_DEBOUNCE_MS) debounced=raw;
  } else if ((now-lastEdge)>=SHOCK_DEBOUNCE_MS && debounced!=raw) {
    debounced = raw;
    if (debounced == (SHOCK_ACTIVE_LOW?LOW:HIGH)) lastShock = now;
  }
  return (lastShock && (now-lastShock <= SHOCK_LATCH_MS));
}

void sensorsBegin() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while(1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(SHOCK_PIN, SHOCK_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
}

SensorData sensorsRead() {
  sensors_event_t a,g,t;
  mpu.getEvent(&a,&g,&t);

  unsigned long now=millis();
  float dt = (lastMs==0) ? 0.02f : (now-lastMs)/1000.0f;
  if (dt <= 0) dt = 0.001f;
  lastMs = now;

  float roll_acc  = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f/PI;
  float pitch_acc = atan2f(-a.acceleration.x,
                    sqrtf(a.acceleration.y*a.acceleration.y+a.acceleration.z*a.acceleration.z))*180.0f/PI;

  float gx = g.gyro.x * 180.0f/PI;
  float gy = g.gyro.y * 180.0f/PI;

  g_roll  = 0.96f*(g_roll  + gx*dt) + 0.04f*roll_acc;
  g_pitch = 0.96f*(g_pitch + gy*dt) + 0.04f*pitch_acc;

  float Atotal = sqrtf(a.acceleration.x*a.acceleration.x +
                       a.acceleration.y*a.acceleration.y +
                       a.acceleration.z*a.acceleration.z);

  SensorData s;
  s.A = Atotal;
  s.roll = g_roll;
  s.pitch= g_pitch;
  s.shock= readShock();
  return s;
}