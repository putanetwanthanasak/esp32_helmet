#include "alerts.h"
#include <config.h>

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0, /*clock=*/DISP_SCL, /*data=*/DISP_SDA, /*reset=*/U8X8_PIN_NONE);

// ===== Cancel button with interrupt (ESP32) =====
volatile bool g_cancelFlag = false;      // ถูกแตะจาก ISR -> ต้องเป็น volatile
static unsigned long g_lastCancelMs = 0; // ดีบาวซ์ฝั่ง main-loop

// ISR ต้องสั้น ๆ และอยู่ใน IRAM
static void IRAM_ATTR cancelISR() {
  g_cancelFlag = true;  // แค่ตั้งธงไว้ ห้ามเรียกฟังก์ชันหนัก ๆ ใน ISR
}

void cancelBegin(){
  // ใช้ INPUT_PULLUP เสมอ แล้วต่อปุ่มลง GND (active-low)
  pinMode(CANCEL_PIN, INPUT_PULLUP);
  // ติดตั้งอินเทอร์รัพท์ขอบตก (FALLING) สำหรับ active-low
  attachInterrupt(digitalPinToInterrupt(CANCEL_PIN), cancelISR, FALLING);
}

// อ่านแล้ว "กินธง" พร้อมดีบาวซ์แบบไม่บล็อก
bool cancelPressed(){
  if (!g_cancelFlag) return false;
  unsigned long now = millis();
  if (now - g_lastCancelMs < CANCEL_DEBOUNCE_MS) {
    // กดยิก ๆ ติดกัน ให้รอครบดีบาวซ์ก่อนค่อยยอมรับครั้งถัดไป
    g_cancelFlag = false;  // กินธงทิ้งเพื่อไม่ให้ลูปเด้งซ้ำ
    return false;
  }
  g_lastCancelMs = now;
  g_cancelFlag = false;    // consume flag
  return true;
}

// Buzzer
static bool b_blink=false, b_on=false;
static unsigned long b_last=0;

static inline void buzWrite(bool on){
  if (BUZZER_ACTIVE_HIGH) digitalWrite(BUZZER_PIN, on?HIGH:LOW);
  else                    digitalWrite(BUZZER_PIN, on?LOW:HIGH);
}
void buzzerBegin(){ pinMode(BUZZER_PIN, OUTPUT); buzWrite(false); }
void buzzerBlinkStart(){ b_blink=true; b_on=false; b_last=millis(); buzWrite(false); }
void buzzerBlinkStop(){ b_blink=false; b_on=false; buzWrite(false); }
void buzzerBlinkUpdate(){
  if (!b_blink) return;
  unsigned long now=millis();
  if (now - b_last >= BUZZER_BLINK_MS){
    b_last = now; b_on = !b_on; buzWrite(b_on);
  }
}

//LED
static bool ledBlink = false;
static bool ledState = false;
static unsigned long lastLed = 0;

void emerLedBegin() {
  pinMode(EMER_LED_PIN, OUTPUT);
  digitalWrite(EMER_LED_PIN, EMER_LED_ACTIVE_HIGH ? LOW : HIGH);
}

void emerLedBlinkStart() {
  ledBlink = true;
  lastLed = millis();
  ledState = false;
}

void emerLedBlinkStop() {
  ledBlink = false;
  ledState = false;
  digitalWrite(EMER_LED_PIN, EMER_LED_ACTIVE_HIGH ? LOW : HIGH);
}

void emerLedBlinkUpdate() {
  if (!ledBlink) return;
  if (millis() - lastLed >= EMER_LED_BLINK_MS) {
    lastLed = millis();
    ledState = !ledState;
    digitalWrite(EMER_LED_PIN, ledState ? HIGH : LOW);
  }
}
// OLED (สไตล์ตามโค้ดเดิม)
void oledBegin(){ u8g2.begin(); }
void oledReady(){
  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,12,"System Ready"); u8g2.sendBuffer();
}

// ===== System Ready with Wi-Fi & GPS status (draw only on change) =====
static int  _prevWifiReady = -1;   // -1 unknown, 0 no, 1 yes
static IPAddress _prevIp;
static int  _prevGpsReady  = -1;   // -1 unknown, 0 no, 1 yes
static int  _prevGpsSats   = -1;

void oledReadyStatus(bool wifiReady, const IPAddress& ip, bool gpsReady, int sats){
  const int w = wifiReady ? 1 : 0;
  const int g = gpsReady  ? 1 : 0;

  // วาดเฉพาะตอนสถานะ/ค่าเปลี่ยน ลดการรีเฟรชจอ
  if (w == _prevWifiReady && g == _prevGpsReady && sats == _prevGpsSats && ip == _prevIp) {
    return;
  }
  _prevWifiReady = w;
  _prevGpsReady  = g;
  _prevGpsSats   = sats;
  _prevIp        = ip;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // บรรทัด 1: หัวเรื่อง
  u8g2.drawStr(0, 12, "System Ready");

  // บรรทัด 2–3: Wi-Fi
  if (!wifiReady) {
    u8g2.drawStr(0, 28, "Wi-Fi: connecting...");
  } else {
    u8g2.drawStr(0, 28, "Wi-Fi: connected");
    char ipbuf[24];
    snprintf(ipbuf, sizeof(ipbuf), "IP: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    u8g2.drawStr(0, 40, ipbuf);
  }

  // บรรทัด 4–5: GPS
  if (!gpsReady) {
    u8g2.drawStr(0, 56, "GPS: Searching...");
  } else {
    char gpsbuf[32];
    snprintf(gpsbuf, sizeof(gpsbuf), "GPS: ready  Sats:%d", sats);
    u8g2.drawStr(0, 56, gpsbuf);
  }

  u8g2.sendBuffer();
}
void oledCrashWaiting(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB12_tr); u8g2.drawStr(0,15,"CRASH!");
  u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0,35,"Waiting 30s...");
  u8g2.sendBuffer();
}
void oledSending(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(0,14,"Sending...");
  u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0,32,"Loading data...");
  u8g2.sendBuffer();
}
void oledFetchFailed(const String& err){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(0,15,"FETCH FAILED");
  u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0,40, err.substring(0,20).c_str());
  u8g2.sendBuffer();
}
void oledHelpSent(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(0,14,"HELP SENT");
  u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0,32,"Press STOP to silence");
  u8g2.sendBuffer();
}


// Scrolling 8 lines
struct ScrollState {
  bool active=false; int yStart=64,yEnd=-80,yCur=64;
  unsigned long startMs=0,lastFrame=0;
  String lines[8];
} sc;

void scrollStart(const String (&lines)[8]){
  for(int i=0;i<8;i++) sc.lines[i]=lines[i];
  sc.active=true; sc.yStart=64; sc.yEnd=-80; sc.yCur=64;
  sc.startMs=millis(); sc.lastFrame=0;
}
void scrollStop(){ sc.active=false; }
bool scrollActive(){ return sc.active; }

void scrollUpdate(){
  if (!sc.active) return;
  unsigned long now = millis();
  const int frameDelay = 100; // 10 fps ช้าลงแต่ลื่นพอ
  if (now - sc.lastFrame < (unsigned long)frameDelay) return;
  sc.lastFrame = now;

  sc.yCur--;
  if (sc.yCur < sc.yEnd) sc.yCur = sc.yStart;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  int y = sc.yCur, H = 10;
  for (int i = 0; i < 8; i++) {
    u8g2.drawStr(0, y, sc.lines[i].substring(0, 20).c_str());
    y += H;
  }
  u8g2.sendBuffer();

  // 🟢 ปล่อย CPU ให้ทำงานอื่น เช่น อ่านปุ่ม
  yield();
}