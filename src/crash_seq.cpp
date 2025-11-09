#include "crash_seq.h"
#include "alerts.h"
#include "gps.h"
#include "line_messaging_api.h"   // LINE Messaging API (push)
#include <config.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Arduino.h>

// ---------- Firebase extern (ถูกประกาศใน main.cpp) ----------
extern FirebaseData   fbdo;
extern FirebaseAuth   auth;
extern FirebaseConfig fbConfig;

// ---------- ข้อมูลผู้ใช้จาก Firebase ----------
String g_firstname, g_lastname, g_blood, g_disease, g_allergy, g_medication, g_emer_name, g_emer_phone;
String g_fetchError = "";

// ---------- สถานะของ Crash Sequence ----------
enum class CrashState : uint8_t {
  IDLE,       // ปกติ
  A_ACTIVE,   // นับถอยหลัง (เสียงกระพริบ + จอ "Waiting …")
  B_ACTIVE,   // ส่ง LINE + ดึงข้อมูล + สกรอลจอ
  HOLDING     // โชว์ "HELP SENT" ค้างจนผู้ใช้กดยกเลิก
};
static CrashState cs = CrashState::IDLE;
static unsigned long tA = 0, tB = 0;   // time anchors
static bool lineSent = false;
static bool dataFetched = false;

// ---------- Helper: แปลง string ของ Firebase -> Arduino String ----------
static String fbGetString(FirebaseJson& json, const char* key) {
  FirebaseJsonData r;
  if (json.get(r, key)) return String(r.stringValue.c_str()); // MB_String -> const char* -> String
  return String();
}

// ---------- ดึงข้อมูลผู้ใช้จาก Firebase (เป็น Arduino String เสมอ) ----------
static bool fetchCrashInfo() {
  g_fetchError = "";

  String path; 
  path.reserve(16 + String(FIREBASE_USER_ID).length());
  path  = "/users/";
  path += String(FIREBASE_USER_ID);

  if (Firebase.get(fbdo, path) && fbdo.dataType() == "json") {
    FirebaseJson json; 
    json.setJsonData(fbdo.stringData());

    g_firstname  = fbGetString(json, "firstname");
    g_lastname   = fbGetString(json, "lastname");
    g_blood      = fbGetString(json, "bloodgroup");
    g_disease    = fbGetString(json, "disease");
    g_allergy    = fbGetString(json, "allergy");
    g_medication = fbGetString(json, "medication");
    g_emer_name  = fbGetString(json, "emergencyname");
    g_emer_phone = fbGetString(json, "emergencyphone");
    return true;
  } else {
    g_fetchError = String(fbdo.errorReason().c_str());
    return false;
  }
}

// ---------- เริ่มสกรอลข้อความ 8 บรรทัดบนจอ ----------
static void startScroll() {
  String L[8];
  L[0] = "First: ";   L[0] += g_firstname;
  L[1] = "Last: ";    L[1] += g_lastname;
  L[2] = "Blood: ";   L[2] += g_blood;
  L[3] = "Disease: "; L[3] += g_disease;
  L[4] = "Allergy: "; L[4] += g_allergy;
  L[5] = "Meds: ";    L[5] += g_medication;
  L[6] = "Contact: "; L[6] += g_emer_name;
  L[7] = "Phone: ";   L[7] += g_emer_phone;
  scrollStart(L);
}

// ---------- รีเซ็ตทุกอย่างกลับสู่โหมดพร้อมตรวจ ----------
static void resetAll() {
  buzzerBlinkStop();
  scrollStop();
  oledReady();
  cs = CrashState::IDLE;
  tA = tB = 0;
  lineSent = false;
  dataFetched = false;
}

// ====== Public APIs =========================================================
void crashSeqBegin() { resetAll(); }
bool crashActive()   { return cs != CrashState::IDLE; }

void crashSeqStart() {
  if (cs != CrashState::IDLE) return;     // กันโดนซ้ำ
  tA = millis();
  buzzerBlinkStart();                      // เสียงกระพริบ (จะดังจนกดยกเลิก)
  oledCrashWaiting();                      // หน้าจอ "CRASH!" + "Waiting …"
  cs = CrashState::A_ACTIVE;
}

void crashSeqUpdate() {
  // ปุ่มยกเลิกมาก่อนเสมอ
  if (cancelPressed()) { resetAll(); return; }

  // งาน background non-blocking
  buzzerBlinkUpdate();
  scrollUpdate();
  gpsPoll();

  const unsigned long now = millis();

  switch (cs) {

    case CrashState::IDLE:
      // ไม่มีอะไรทำ
      break;

    case CrashState::A_ACTIVE:
      // ครบเวลาหน่วง -> เข้าสเตจส่ง LINE + ดึงข้อมูล
      if (now - tA >= CRASH_DELAY_MS) {
        cs = CrashState::B_ACTIVE;
        tB = now;
        lineSent = false;
        dataFetched = false;
        oledSending();                    // "Sending..." / "Loading..."
      }
      break;

    case CrashState::B_ACTIVE:
      // 1) ส่ง LINE (ครั้งเดียว)
      if (!lineSent) {
        GPSFix fix = gpsGetFix();
        GPSFix raw = gpsGetRaw();
        bool ok = false;

        if (fix.valid) {
          String extra; extra.reserve(32);
          extra  = "sats="; extra += String(fix.sats);
          extra += " age="; extra += String(fix.age_ms);
          extra += "ms";
          ok = lineMsgPushCrash(LINE_USER_ID, fix.lat, fix.lon, LINE_TITLE, extra);
        } 
        else if (raw.valid) {
          String extra; extra.reserve(40);
          extra  = "approx sats="; extra += String(raw.sats);
          extra += " age=";        extra += String(raw.age_ms);
          extra += "ms";
          ok = lineMsgPushCrash(LINE_USER_ID, raw.lat, raw.lon, LINE_TITLE, extra);
        } 
        else {
          String msg = String(LINE_TITLE);
          msg += "\nNo GPS fix.\nCheck device location.";
          ok = lineMsgPushText(LINE_USER_ID, msg);
        }

        Serial.println(ok ? "LINE (Messaging API): sent" : "LINE (Messaging API): failed");
        lineSent = true;
      }

      // 2) ดึงข้อมูลจาก Firebase + เริ่มสกรอล (ครั้งเดียว)
      if (!dataFetched) {
        dataFetched = true;
        if (fetchCrashInfo()) startScroll();
        else                  oledFetchFailed(g_fetchError);
      }

      // 3) เมื่อสกรอลครบหรือเกินเวลา -> ไป HOLDING
      if (!scrollActive() || (now - tB > POST_B_ACTION_TIMEOUT)) {
        oledHelpSent();                   // แสดง "HELP SENT"
        cs = CrashState::HOLDING;
      }
      break;

    case CrashState::HOLDING:
      // ค้างเสียง/จอจนกว่าจะกดยกเลิก (cancelPressed ด้านบนจะ reset)
      break;
  }
}