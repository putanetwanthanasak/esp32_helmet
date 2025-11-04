#include <Arduino.h>
#include <config.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include "Line_MessagingAPI.h"
#include "secrets.h"

// ========== GLOBALS ==========
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <U8g2lib.h>

// --- !!! สวิตช์หลัก: 1 = โหมดจำลอง, 0 = โหมดใช้งานจริง !!! ---
#define SIMULATION_MODE 0


//=================================================================
// == 1. GLOBAL VARIABLES & OBJECTS (ตัวแปรและ Object ส่วนกลาง)
//=================================================================

Adafruit_MPU6050 mpu;
static float g_roll_deg  = 0.0f;
static float g_pitch_deg = 0.0f;
static unsigned long g_last_ms = 0;

// --- ตัวแปรส่วนกลางของโปรเจกต์ (U8g2) ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// ใหม่ (SW I2C แยกพินจอ ไม่ยุ่งกับ Wire/Sensors)
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0,
  /* clock=*/ DISP_SCL,
  /* data=*/  DISP_SDA,
  /* reset=*/ U8X8_PIN_NONE);

// --- ตัวแปร Global สำหรับเก็บข้อมูลผู้ใช้ (ดึงจาก Firebase) ---
String g_firstname, g_lastname, g_blood, g_disease, g_allergy, g_medication, g_emer_name, g_emer_phone;
String g_fetchError = ""; // เก็บข้อความ Error ถ้าดึงข้อมูลไม่สำเร็จ


//=================================================================
// == 2. SENSOR LOGIC FUNCTIONS (ฟังก์ชันอ่านค่า Sensor)
//=================================================================
// (เราจะเก็บฟังก์ชันเหล่านี้ไว้ แม้ในโหมดจำลอง เพื่อให้โค้ดพร้อมใช้งาน)

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
  static unsigned long lastEdgeMs = 0;           // เวลาเปลี่ยนแปลงของสัญญาณดิบครั้งล่าสุด
  static int lastRaw = -1;                        // ค่าดิบครั้งก่อน (-1 = ยังไม่เริ่ม)
  static int debounced = -1;                      // ค่าที่ดีบาวซ์แล้ว
  static unsigned long lastShockMs = 0;           // เวลาที่เกิด shock ครั้งล่าสุด (หลังดีบาวซ์)
  
  const int activeLevel = SHOCK_ACTIVE_LOW ? LOW : HIGH;
  const unsigned long now = millis();

  // อ่านค่า raw ปัจจุบัน
  const int raw = digitalRead(SHOCK_PIN);

  // 1) ติดตามการเปลี่ยนของ raw เพื่อเริ่มจับเวลาดีบาวซ์
  if (raw != lastRaw) {
    lastRaw = raw;
    lastEdgeMs = now;     // เริ่มจับเวลาเมื่อมีการเปลี่ยนแปลง
  }

  // 2) ถ้าค่า raw คงที่นานเกิน DEBOUNCE → ยอมรับเป็นค่า debounced ใหม่
  if (debounced == -1) {
    // ครั้งแรก: ตั้งต้นด้วยค่า raw หลังผ่าน DEBOUNCE เพื่อกัน false trigger
    if ((now - lastEdgeMs) >= SHOCK_DEBOUNCE_MS) {
      debounced = raw;
    }
  } else if ((now - lastEdgeMs) >= SHOCK_DEBOUNCE_MS && debounced != raw) {
    debounced = raw;

    // 3) ตรวจจับ "เหตุการณ์" เมื่อค่าที่ดีบาวซ์เปลี่ยนเข้า activeLevel
    if (debounced == activeLevel) {
      lastShockMs = now;  // เกิด shock (ขอบที่ดีบาวซ์แล้ว)
    }
  }

  // 4) Latch: ภายในหน้าต่างเวลาหลัง shock ยังถือว่า "มีช็อก"
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
  if (impactNow || shockNow) {
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


//=================================================================
// == 3. SYSTEM & HELPER FUNCTIONS (ฟังก์ชันช่วยและตั้งค่าระบบ)
//=================================================================

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

void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void setupFirebase() {
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase connected");
}

void setupDisplay() {
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // <--- ใช้ Font ขนาดเล็ก (8px)
  u8g2.drawStr(0, 12, "System Ready");
  u8g2.sendBuffer(); 
  Serial.println("OLED (U8g2) Initialized");
}


//=================================================================
// == 4. CORE APP LOGIC (ฟังก์ชันหลักของโปรแกรม)
//=================================================================

// --- ฟังก์ชันสำหรับดึงข้อมูล (Fetch) ---
// (จะคืนค่า true ถ้าสำเร็จ, false ถ้าล้มเหลว)
bool fetchCrashInfo() {
  g_fetchError = ""; // เคลียร์ Error เก่า
  Serial.println("Fetching user data from Firebase...");
  
  // แสดง "Loading..." บนจอ
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB12_tr); // Font ใหญ่สำหรับ "CRASH!"
  u8g2.drawStr(0, 15, "CRASH!");
  u8g2.setFont(u8g2_font_ncenB08_tr); // Font เล็กสำหรับ "Loading"
  u8g2.drawStr(0, 35, "Loading data...");
  u8g2.sendBuffer();

  String path = String("/users/");
  path.concat(String(FIREBASE_USER_ID));
  
  if (Firebase.get(fbdo, path)) {
    if (fbdo.dataType() == "json") {
      FirebaseJson json; 
      json.setJsonData(fbdo.stringData()); 
      FirebaseJsonData result;
      
      // --- ดึงข้อมูลทั้งหมดมาเก็บในตัวแปร Global ---
      json.get(result, "firstname");      g_firstname = result.stringValue;
      json.get(result, "lastname");       g_lastname = result.stringValue;
      json.get(result, "bloodgroup");     g_blood = result.stringValue;
      json.get(result, "disease");        g_disease = result.stringValue;
      json.get(result, "allergy");        g_allergy = result.stringValue;
      json.get(result, "medication");     g_medication = result.stringValue;
      json.get(result, "emergencyname");  g_emer_name = result.stringValue;
      json.get(result, "emergencyphone"); g_emer_phone = result.stringValue;

      Serial.println("Data fetched!");
      return true; // ดึงข้อมูลสำเร็จ
      
    } else {
      Serial.println("Failed to parse JSON");
      g_fetchError = "Data Error";
      return false; // ดึงข้อมูลล้มเหลว
    }
  } else {
    Serial.print(F("Firebase get failed: "));
    Serial.println(fbdo.errorReason());
    g_fetchError = "Firebase Error";
    return false; // ดึงข้อมูลล้มเหลว
  }
}

// --- ฟังก์ชันสำหรับแสดงผลแบบ "Text เลื่อน" (Scrolling) ---
void displayScrollingInfo() {
  const long totalDisplayTime = 30000; // แสดงผลทั้งหมด 30 วินาที
  const int lineHeight = 10; // ความสูงของแต่ละบรรทัด (font 8px + 2px spacing)
  const int totalLines = 8;
  const int totalHeight = totalLines * lineHeight; // 80px (ความสูง Text ทั้งหมด)
  const int screenHeight = 64; // ความสูงจอ

  int yStart = screenHeight; // เริ่มที่ 64 (นอกจอด้านล่าง)
  int yEnd = -totalHeight;   // เลื่อนจนสุดที่ -80 (นอกจอด้านบน)
  int yCurrent = yStart;

  unsigned long startTime = millis();
  unsigned long lastFrameTime = 0;
  const int frameDelay = 50; // 50ms = 20fps (20 เฟรมต่อวินาที)

  // วนลูปนี้เป็นเวลา 30 วินาที
  while (millis() - startTime < totalDisplayTime) {
    
    unsigned long now = millis();
    // รอให้ครบ 50ms ก่อนวาดเฟรมถัดไป
    if (now - lastFrameTime < frameDelay) {
      delay(1); // หน่วงเวลาเล็กน้อย
      continue; 
    }
    lastFrameTime = now;
    
    yCurrent--; // เลื่อนขึ้น 1 pixel
    if (yCurrent < yEnd) {
      yCurrent = yStart; // วนกลับไปเริ่มใหม่
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr); // Font เล็ก

    int y = yCurrent;

    // --- สร้าง String โดยใช้ .concat() (ป้องกัน Error) ---
    String line1 = "First: "; line1.concat(g_firstname);
    String line2 = "Last: ";  line2.concat(g_lastname);
    String line3 = "Blood: "; line3.concat(g_blood);
    String line4 = "Disease: "; line4.concat(g_disease);
    String line5 = "Allergy: "; line5.concat(g_allergy);
    String line6 = "Meds: ";    line6.concat(g_medication);
    String line7 = "Contact: "; line7.concat(g_emer_name);
    String line8 = "Phone: ";   line8.concat(g_emer_phone);

    // --- วาด 8 บรรทัด (ตัดข้อความยาวๆ ที่ 20 ตัวอักษร) ---
    u8g2.drawStr(0, y, line1.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line2.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line3.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line4.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line5.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line6.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line7.substring(0, 20).c_str()); y += lineHeight;
    u8g2.drawStr(0, y, line8.substring(0, 20).c_str());
    
    u8g2.sendBuffer();
  }
}

// --- ฟังก์ชันสำหรับแสดงผลเมื่อดึงข้อมูลล้มเหลว ---
void displayFetchError() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(0, 15, "FETCH FAILED");
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 40, g_fetchError.c_str()); // แสดง Error ที่เก็บไว้
  u8g2.sendBuffer();
}


//=================================================================
// == 5. ARDUINO SETUP & LOOP (ฟังก์ชันหลัก Setup และ Loop)
//=================================================================

void setup() {
  Serial.begin(115200);
  
  setupDisplay(); // <--- ใช้ U8g2
  setupWiFi();
  setupFirebase();
  
  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(SHOCK_PIN, SHOCK_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
  delay(50);

  // --- !!! ส่วนนี้จะทำงานตาม SIMULATION_MODE !!! ---
  #if SIMULATION_MODE == 1
    // โหมดจำลอง: ข้ามการเช็ค MPU
    Serial.println(F("!!! MPU6050 check SKIPPED for simulation !!!"));
  #else
    // โหมดใช้งานจริง: ตรวจสอบ MPU
    if (!mpu.begin()) {
      Serial.println(F("MPU6050 not found!"));
      // แสดงผลบนจอว่า Error
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 12, "MPU6050 Error!");
      u8g2.drawStr(0, 24, "Check wiring.");
      u8g2.sendBuffer();
      while (1) delay(10); // หยุดการทำงาน
    }
    Serial.println(F("MPU6050 Initialized."));
    // ตั้งค่า MPU
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  #endif
  // --- จบส่วน MPU setup ---

  delay(100);

  Serial.println(F("System Ready."));
  pinMode(BUZZER_PIN, OUTPUT);
  controlBuzzer(false);
}

void loop() {
  
  // bool crashDetected = false; // สร้าง Flag เพื่อเก็บสถานะการชน

  #if SIMULATION_MODE == 1
    // --- !!! โหมดจำลอง !!! ---
    
    Serial.println("Simulating crash event in 3 seconds...");
    delay(3000); // รอ 3 วิ
    crashDetected = true; // บังคับให้ชน

  #else
    // --- !!! โหมดใช้งานจริง !!! ---
    
    // 1. อ่าน Sensor
    IMURead imu = readIMU();
    bool impact = detectImpact(imu.Atotal_ms2);
    bool tilt   = detectTilt(imu.roll_deg, imu.pitch_deg);
    bool shock  = readShockEvent();
    
    // 2. สรุปผล
    bool crashed = crashDecision(impact, tilt, shock);

  #endif
  Serial.print("A:"); Serial.print(imu.Atotal_ms2, 2);
  Serial.print("  R:"); Serial.print(imu.roll_deg, 1);
  Serial.print("  P:"); Serial.print(imu.pitch_deg, 1);
  Serial.print("  | impact="); Serial.print(impact);
  Serial.print(" tilt="); Serial.print(tilt);
  Serial.print(" shock="); Serial.print(shock);
  Serial.print("  => CRASH=");
  Serial.println(crashed ? "YES" : "no");

  // --- ( Logic การจัดการการชน (ใช้ร่วมกันทั้ง 2 โหมด) ) ---
  if (crashed) {
    triggerCrashAlert(); // สั่ง Buzzer ดัง
     double lat = 13.6515;      // ใส่ค่าจาก GPS ของคุณ
     double lon = 100.4943;     // ใส่ค่าจาก GPS ของคุณ
    lineSendCrash(lat, lon);
    
    if (fetchCrashInfo()) {
      // ถ้าดึงข้อมูลสำเร็จ (true)
      Serial.println("Fetch OK. Starting scrolling display...");
      displayScrollingInfo(); // <--- เรียกใช้ฟังก์ชัน "Text เลื่อน"
      
    } else {
      // ถ้าดึงข้อมูลล้มเหลว (false)
      Serial.println("Fetch FAILED. Displaying error.");
      displayFetchError(); // <--- แสดงหน้าจอ Error
      delay(30000); // <--- รอ 30 วินาทีเหมือนกัน
    }
    
    Serial.println("Crash sequence complete. Resetting loop.");
    
    // อัปเดตจอให้กลับเป็น "System Ready"
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 12, "System Ready");
    u8g2.sendBuffer();
  }
  
  // หน่วงเวลาเล็กน้อยใน loop หลัก (สำหรับโหมดใช้งานจริง)
  delay(100); 
}