// ตัวแปรพินเอามาใส่ไว้ในไฟล์นี้
#ifndef CONFIG_H
#define CONFIG_H

// 1. ดึงค่า secret ทั้งหมด (WiFi, Auth, UserID) จาก secrets.h
#include "secrets.h"

// ================= ตั้งค่า I2C =================
#define I2C_SDA               21    // SDA pin on ESP32
#define I2C_SCL               22    // SCL pin on ESP32

// =============== DISPLAY (OLED) PINS ===============
#define DISP_SDA  19   // เลือก GPIO ที่ว่างและขับได้
#define DISP_SCL  23   // อย่าใช้ 34–39 (input-only)

// ================= SHOCK SENSOR =================
#define SHOCK_PIN             34    // ขาเชื่อมต่อ shock sensor
#define SHOCK_ACTIVE_LOW      1     // ถ้า shock sensor เป็น active LOW ให้เป็น 1, ถ้า active HIGH ให้เป็น 0

// ================= THRESHOLD =================
#define IMPACT_THRESH_MS2     15.0f  // แรงรวม (m/s²) เกินค่านี้ถือว่ามีแรงกระแทก
#define TILT_THRESH_DEG       45.0f  // มุมเอียงเกินนี้ถือว่าเอียงผิดปกติ

// ================= TIMING =================
#define ARM_WINDOW_MS         1000   // ระยะเวลา (ms) หลังจากตรวจพบแรงกระแทก/ช็อก ที่จะตรวจต่อว่าล้มไหม
#define TILT_HOLD_MS          1500   // ต้องเอียงค้างอย่างน้อยเท่านี้ถึงจะถือว่าล้มจริง
#define SHOCK_DEBOUNCE_MS     30     // เวลาดีบั๊วน์ (ms) สำหรับ shock sensor
#define SHOCK_LATCH_MS        300    // ระยะเวลาที่ shock sensor ถูกล็อกเหตุการณ์ล่าสุดไว้

// ================= Buzzer =================
#define BUZZER_PIN 4        // ขา GPIO ต่อ buzzer
#define BUZZER_ACTIVE_HIGH 0 // ถ้า buzzer ดังเมื่อ HIGH ให้ = 1, ถ้าดังเมื่อ LOW ให้ = 0
#define BUZZER_DURATION 1000 // เวลาเปิดเสียง (มิลลิวินาที)

// ================= FIREBASE (ส่วน Public) =================
#define FIREBASE_HOST         "https://helmetsafty-default-rtdb.asia-southeast1.firebasedatabase.app" 

#endif