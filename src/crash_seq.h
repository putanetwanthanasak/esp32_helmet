#ifndef CRASH_SEQ_H
#define CRASH_SEQ_H

#include <Arduino.h>

// ฟังก์ชันที่ให้ main.cpp เรียกได้
void crashSeqBegin();     // เริ่มต้นระบบ
bool crashActive();       // เช็กว่ากำลังอยู่ในสถานะแจ้งเหตุไหม
void crashSeqStart();     // เริ่ม sequence แจ้งเหตุ (ตอนตรวจพบการล้ม)
void crashSeqUpdate();    // เรียกใน loop() ตลอดเพื่ออัปเดตสถานะ

#endif