#pragma once
#include <Arduino.h>

// ---------------- LINE Messaging API ----------------
// ใช้สำหรับส่งข้อความด้วย LINE Messaging API (push message)

// สร้าง JSON ของข้อความ (type: text)
String lineBuildText(const String &msg);

// สร้าง JSON ของข้อความ location
String lineBuildLocation(double lat, double lon, const String &title = "ตำแหน่งเกิดเหตุ", const String &addr = "แตะเพื่อเปิดแผนที่");

// รวมข้อความหลายบับเบิล (array string เช่น text+","+location)
String lineBuildPushPayload(const String &to, const String &messageJsonArray);

// ฟังก์ชันยิง HTTPS POST ไปยัง LINE API
bool linePush(const String &payload);

// ฟังก์ชันสะดวกใช้ทั่วไป
bool lineSendText(const String &to, const String &text);
bool lineSendLocation(const String &to, double lat, double lon, const String &title = "ตำแหน่งเกิดเหตุ", const String &addr = "แตะเพื่อเปิดแผนที่");

// ฟังก์ชันหลัก “แจ้งเตือนโดนชน + พิกัด GPS”
bool lineSendCrash(double lat, double lon);