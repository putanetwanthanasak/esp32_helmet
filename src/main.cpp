#include <Arduino.h>
#include <config.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>


// ========== GLOBALS ==========
Adafruit_MPU6050 mpu;
static float g_roll_deg  = 0.0f;
static float g_pitch_deg = 0.0f;
static unsigned long g_last_ms = 0;

// ====== 1) อ่าน IMU แล้วคืน "คุณลักษณะ" ======
struct IMURead {
  float Atotal_ms2;  // แรงรวม m/s^2
  float roll_deg;    // เอียงซ้าย/ขวา
  float pitch_deg;   // ก้ม/เงย
};

IMURead readIMU() {
  // Complementary filter อย่างเบา ๆ
  const float ALPHA = 0.96f;

  if (g_last_ms == 0) g_last_ms = millis();
  unsigned long now = millis();
  float dt = (now - g_last_ms) / 1000.0f;
  if (dt <= 0) dt = 0.001f;
  g_last_ms = now;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  // มุมจาก Acc
  float roll_acc  = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
  float pitch_acc = atan2f(-a.acceleration.x,
                           sqrtf(a.acceleration.y*a.acceleration.y +
                                 a.acceleration.z*a.acceleration.z)) * 180.0f / PI;

  // Gyro (rad/s -> deg/s)
  float gx = g.gyro.x * 180.0f / PI;  // หมุนรอบแกน X -> roll
  float gy = g.gyro.y * 180.0f / PI;  // หมุนรอบแกน Y -> pitch

  // integrate + fuse
  float roll_gyro  = g_roll_deg  + gx * dt;
  float pitch_gyro = g_pitch_deg + gy * dt;
  g_roll_deg  = ALPHA * roll_gyro  + (1.0f - ALPHA) * roll_acc;
  g_pitch_deg = ALPHA * pitch_gyro + (1.0f - ALPHA) * pitch_acc;

  // แรงรวม
  float Atotal = sqrtf(a.acceleration.x*a.acceleration.x +
                       a.acceleration.y*a.acceleration.y +
                       a.acceleration.z*a.acceleration.z);

  return IMURead{ Atotal, g_roll_deg, g_pitch_deg };
}

// ====== 2) ฟังก์ชันตรวจ "กระแทก" จากแรงรวม ======
bool detectImpact(float Atotal_ms2) {
  return (Atotal_ms2 > IMPACT_THRESH_MS2);
}

// ====== 3) ฟังก์ชันตรวจ "เอียงผิดปกติ" ======
bool detectTilt(float roll_deg, float pitch_deg) {
  return (fabs(roll_deg) >= TILT_THRESH_DEG) || (fabs(pitch_deg) >= TILT_THRESH_DEG);
}

// ====== 4) อ่าน Shock sensor แบบ debounce + latch (คืน TRUE เมื่อเพิ่งมีเหตุการณ์) ======
bool readShockEvent() {
  static unsigned long lastChangeMs = 0;
  static int lastStable = SHOCK_ACTIVE_LOW ? HIGH : LOW; // เริ่มนิ่ง
  static unsigned long lastShockMs = 0;

  int raw = digitalRead(SHOCK_PIN);
  int activeLevel = SHOCK_ACTIVE_LOW ? LOW : HIGH;

  unsigned long now = millis();

  // debounce เปลี่ยนสถานะ
  if (raw != lastStable && (now - lastChangeMs) > SHOCK_DEBOUNCE_MS) {
    lastChangeMs = now;
    lastStable = raw;
    if (lastStable == activeLevel) {
      lastShockMs = now; // จับเหตุการณ์ช็อก
      return true;
    }
  }

  // ภายในหน้าต่าง latch ยังถือว่า "มีช็อกล่าสุด"
  if (lastShockMs && (now - lastShockMs) <= SHOCK_LATCH_MS) {
    return true;
  }
  return false;
}

// ====== 5) ฟังก์ชันสรุปการชน (รวม Impact + Tilt + Shock) ======
bool crashDecision(bool impactNow, bool tiltNow, bool shockNow) {
  static bool armed = false;
  static unsigned long armUntilMs = 0;
  static unsigned long tiltSinceMs = 0;

  unsigned long now = millis();

  // ถ้ามี impact หรือ shock ตอนนี้ → เปิดโหมดตรวจจับใหม่
  if (impactNow && shockNow) {
    armed = true;
    armUntilMs = now + ARM_WINDOW_MS;
  }

  // ถ้าไม่มี impact/shock เลย และไม่มี tilt → reset สถานะทั้งหมด
  if (!impactNow && !shockNow && !tiltNow) {
    armed = false;
    tiltSinceMs = 0;
  }

  // ถ้าเอียงอยู่ → เริ่มจับเวลาเอียงค้าง
  if (tiltNow) {
    if (tiltSinceMs == 0) tiltSinceMs = now;
  } else {
    tiltSinceMs = 0;
  }

  // ตรวจภายในช่วง armed เท่านั้น
  if (armed && (now <= armUntilMs)) {
    if (tiltSinceMs && (now - tiltSinceMs) >= TILT_HOLD_MS) {
      armed = false;
      tiltSinceMs = 0;
      return true; // ล้มจริง
    }
  } else if (armed && (now > armUntilMs)) {
    // หมดเวลาหน้าต่างตรวจจับ
    armed = false;
    tiltSinceMs = 0;
  }

  return false; // ยังไม่ล้ม
}

void controlBuzzer(bool state) {
  if (BUZZER_ACTIVE_HIGH) {
    digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
  } else {
    digitalWrite(BUZZER_PIN, state ? LOW : HIGH);
  }
}
void triggerCrashAlert() {
  Serial.println("🚨 Crash detected! Turning on buzzer...");
  controlBuzzer(true);
  delay(BUZZER_DURATION);
  controlBuzzer(false);
}



// ========== Arduino setup/loop ==========
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(SHOCK_PIN, SHOCK_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
  delay(50);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);

  Serial.println("Ready.");
  pinMode(BUZZER_PIN, OUTPUT);
  controlBuzzer(false);  // เริ่มต้นปิดเสียงไว้ก่อน
}

void loop() {
  // อ่านคุณลักษณะจาก IMU
  IMURead imu = readIMU();

  // ตัดสินย่อย
  bool impact = detectImpact(imu.Atotal_ms2);
  bool tilt   = detectTilt(imu.roll_deg, imu.pitch_deg);
  bool shock  = readShockEvent();

  // สรุปเป็น "ชนจริง" ?
  bool crashed = crashDecision(impact, tilt, shock);

  // DEBUG/ต่อยอด
  // Serial.print("A:"); Serial.print(imu.Atotal_ms2, 2);
  // Serial.print("  R:"); Serial.print(imu.roll_deg, 1);
  // Serial.print("  P:"); Serial.print(imu.pitch_deg, 1);
  // Serial.print("  | impact="); Serial.print(impact);
  // Serial.print(" tilt="); Serial.print(tilt);
  // Serial.print(" shock="); Serial.print(shock);
  // Serial.print("  => CRASH=");
  // Serial.println(crashed ? "YES" : "no");

  if (crashed) {
    // TODO: เปิด Buzzer/ไฟฉุกเฉิน/เริ่ม countdown ส่ง GPS
    // triggerBuzzer(); startSOS(); sendGPS();
    triggerCrashAlert();
  }

  delay(100);
}