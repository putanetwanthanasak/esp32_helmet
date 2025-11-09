#ifndef CONFIG_H
#define CONFIG_H

#include "secrets.h"

// ================= I2C =================
#define I2C_SDA 21
#define I2C_SCL 22

// ================= OLED (U8g2 SW I2C แยกบัส) =================
#define DISP_SDA 19
#define DISP_SCL 23
#define DISP_SCROLL_FPS        20
#define DISP_SCROLL_TOTAL_MS   30000UL

// ================= SHOCK SENSOR =================
// GPIO34 เป็น input-only → ถ้าใช้ ACTIVE_LOW ต้องมี pull-up ภายนอก
#define SHOCK_PIN         34
#define SHOCK_ACTIVE_LOW  1
#define SHOCK_DEBOUNCE_MS 30
#define SHOCK_LATCH_MS    300

// ================= MPU THRESHOLDS =================
#define IMPACT_A_T_MS2    15.0f   // |a| เกินถือว่าหนัก
#define IMPACT_JERK_T     20.0f   // Δ|a|/Δt
#define OMEGA_T_DPS       500.0f  // max(|Δroll|,|Δpitch|)/Δt
#define TILT_THRESH_DEG   45.0f   // เอียงเกินนี้ถือว่าอันตราย
#define TILT_HOLD_MS      1500    // ต้องค้างเอียงนานเท่านี้
#define ARM_WINDOW_MS     1000    // เวลาหลัง impact เพื่อจับ tilt

// ================= FALL DETECTION EXTRAS =================
#define INACT_MS          3000
#define TILT_RECOVER_DEG  25.0f

// ================= BUZZER =================
#define BUZZER_PIN        4
#define BUZZER_ACTIVE_HIGH 0
#define BUZZER_BLINK_MS   500

// ================= CANCEL BUTTON =================
#define CANCEL_PIN        25
#define CANCEL_ACTIVE_LOW 1
#define CANCEL_DEBOUNCE_MS 100

// ================= CRASH SEQUENCE =================
#define CRASH_DELAY_MS     30000UL  // A → B 30 วินาที
#define POST_B_ACTION_TIMEOUT 60000UL

// ================= GPS =================
#define GPS_RX_PIN         16    // ESP32 RX <= GPS TX
#define GPS_TX_PIN         17    // ESP32 TX => GPS RX (มักไม่ต้องใช้)
#define GPS_BAUD           9600
#define GPS_MIN_SATS       3
#define GPS_MAX_AGE_MS     1500

// ================= LINE =================
#define LINE_ENABLE        1
#define LINE_TITLE         "🚨 Helmet Alert"

// ================= FIREBASE =================
#define FIREBASE_HOST "https://helmetsafty-default-rtdb.asia-southeast1.firebasedatabase.app"

// ================= DEBUG =================
#define DEBUG_SERIAL          1
#define DEBUG_IMU_PERIOD_MS   200

#endif