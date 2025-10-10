// ตัวแปรพินเอามาใส่ไว้ในไฟล์นี้
#ifndef CONFIG_H
#define CONFIG_H

// ================= I2C CONFIG =================
#define I2C_SDA               21    // SDA pin on ESP32
#define I2C_SCL               22    // SCL pin on ESP32

// ================= SHOCK SENSOR =================
#define SHOCK_PIN             19    // ขาเชื่อมต่อ shock sensor
#define SHOCK_ACTIVE_LOW      1     // ถ้า shock sensor เป็น active LOW ให้เป็น 1, ถ้า active HIGH ให้เป็น 0

// ================= THRESHOLD =================
#define IMPACT_THRESH_MS2     20.0f  // แรงรวม (m/s²) เกินค่านี้ถือว่ามีแรงกระแทก
#define TILT_THRESH_DEG       60.0f  // มุมเอียงเกินนี้ถือว่าเอียงผิดปกติ

// ================= TIMING =================
#define ARM_WINDOW_MS         3000   // ระยะเวลา (ms) หลังจากตรวจพบแรงกระแทก/ช็อก ที่จะตรวจต่อว่าล้มไหม
#define TILT_HOLD_MS          1500   // ต้องเอียงค้างอย่างน้อยเท่านี้ถึงจะถือว่าล้มจริง
#define SHOCK_DEBOUNCE_MS     30     // เวลาดีบั๊วน์ (ms) สำหรับ shock sensor
#define SHOCK_LATCH_MS        500    // ระยะเวลาที่ shock sensor ถูกล็อกเหตุการณ์ล่าสุดไว้


#endif