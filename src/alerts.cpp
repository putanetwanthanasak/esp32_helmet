#include "alerts.h"
#include <config.h>

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0, /*clock=*/DISP_SCL, /*data=*/DISP_SDA, /*reset=*/U8X8_PIN_NONE);

// Cancel
static int c_lastRaw=-1, c_debounced=-1;
static unsigned long c_lastEdge=0;

void cancelBegin(){
  pinMode(CANCEL_PIN, CANCEL_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
}
bool cancelPressed(){
  unsigned long now=millis();
  int raw = digitalRead(CANCEL_PIN);
  if (raw != c_lastRaw){ c_lastRaw=raw; c_lastEdge=now; }
  if (c_debounced==-1){
    if (now - c_lastEdge >= CANCEL_DEBOUNCE_MS) c_debounced=raw;
    return false;
  }
  if ((now-c_lastEdge)>=CANCEL_DEBOUNCE_MS && c_debounced != raw){
    c_debounced = raw;
    const int active = CANCEL_ACTIVE_LOW ? LOW : HIGH;
    if (c_debounced == active) return true;
  }
  return false;
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

// OLED (สไตล์ตามโค้ดเดิม)
void oledBegin(){ u8g2.begin(); }
void oledReady(){
  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,12,"System Ready"); u8g2.sendBuffer();
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
  unsigned long now=millis();
  const int frameDelay = 1000/ DISP_SCROLL_FPS;
  if (now - sc.startMs > DISP_SCROLL_TOTAL_MS){ sc.active=false; return; }
  if (now - sc.lastFrame < (unsigned long)frameDelay) return;
  sc.lastFrame = now;

  sc.yCur--; if (sc.yCur < sc.yEnd) sc.yCur = sc.yStart;

  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
  int y=sc.yCur, H=10;
  for(int i=0;i<8;i++) u8g2.drawStr(0,y, sc.lines[i].substring(0,20).c_str()), y+=H;
  u8g2.sendBuffer();
}