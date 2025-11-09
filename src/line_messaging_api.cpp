#include "line_messaging_api.h"
#include <config.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// --- ช่วย escape ข้อความให้เป็น JSON ปลอดภัย ---
static String jsonEscape(const String& s){
  String o; o.reserve(s.length() + 16);
  for (size_t i=0;i<s.length();++i){
    char c = s[i];
    switch (c){
      case '\"': o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\b': o += "\\b";  break;
      case '\f': o += "\\f";  break;
      case '\n': o += "\\n";  break;
      case '\r': o += "\\r";  break;
      case '\t': o += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) {            // control chars
          char buf[7]; // \u00XX
          sprintf(buf,"\\u%04X",(uint8_t)c);
          o += buf;
        } else {
          o += c; // UTF-8 ผ่านได้
        }
    }
  }
  return o;
}

// --- ส่งข้อความธรรมดา ---
bool lineMsgPushText(const String& userId, const String& text){
  if (WiFi.status()!=WL_CONNECTED) return false;

  // NOTE: ต้องใช้ Channel Access Token ของ Messaging API (ไม่ใช่ Notify)
  const char* endpoint = "https://api.line.me/v2/bot/message/push";
  String payload;
  payload.reserve(text.length() + userId.length() + 64);
  payload  = "{";
  payload += "\"to\":\"";        payload += jsonEscape(userId); payload += "\",";
  payload += "\"messages\":[{\"type\":\"text\",\"text\":\"";
  payload += jsonEscape(text);
  payload += "\"}]";
  payload += "}";

  WiFiClientSecure cli; cli.setInsecure();
  HTTPClient http;
  if (!http.begin(cli, endpoint)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + LINE_CHANNEL_TOKEN); // <- ดูด้านล่าง

  int code = http.POST(payload);
  String resp = http.getString();
  http.end();

  Serial.printf("[LINE] Push %d -> %s\n", code, resp.c_str());
  return (code==200);
}

// --- ส่งข้อความ crash พร้อมพิกัดในข้อความ ---
bool lineMsgPushCrash(const String& userId, double lat, double lon,
                      const String& title, const String& extra)
{
  String msg; 
  msg.reserve(128);
  msg  = title;                        // เช่น "🚨 Crash Detected"
  msg += "\nLocation: ";
  msg += String(lat,6); msg += ","; msg += String(lon,6);
  msg += "\nhttps://maps.google.com/?q=";
  msg += String(lat,6); msg += ","; msg += String(lon,6);
  if (extra.length()){
    msg += "\n"; msg += extra;         // เช่น "sats=7 age=234ms"
  }
  return lineMsgPushText(userId, msg);
}