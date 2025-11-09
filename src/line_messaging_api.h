#pragma once
#include <Arduino.h>

/**
 * ส่ง Push Message แบบข้อความธรรมดา (LINE Messaging API)
 * @param userId  LINE userId ของผู้รับ (ขึ้นต้นด้วย U...)
 * @param message ข้อความ
 * @return true ถ้าส่งสำเร็จ
 */
bool lineMsgPushText(const String& userId, const String& message);

/**
 * ส่งแจ้งเตือนเหตุล้ม พร้อมพิกัดที่ส่งมาเอง
 * @param userId LINE userId ของผู้รับ
 * @param lat    ละติจูด
 * @param lon    ลองจิจูด
 * @param title  หัวข้อ (เช่น "🚨 Helmet Alert")
 * @param extra  ข้อความเพิ่มเติม
 */
bool lineMsgPushCrash(const String& userId, double lat, double lon,
                      const String& title = "🚨 Helmet Alert",
                      const String& extra = "");

/**
 * ส่งแจ้งเตือนเหตุล้ม โดย "ดึงพิกัดจาก GPS โมดูล" ภายในฟังก์ชัน
 * ต้องมี gps.h (gpsBegin/gpsPoll/gpsGetFix/gpsGetRaw) ในโปรเจ็กต์อยู่แล้ว
 * @param userId LINE userId ของผู้รับ
 * @param title  หัวข้อ
 * @param extra  ข้อความเพิ่มเติม
 */
bool lineMsgPushCrashWithGPS(const String& userId,
                             const String& title = "🚨 Helmet Alert",
                             const String& extra = "");