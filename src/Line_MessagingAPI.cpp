#include <Arduino.h>
#include <HTTPClient.h>
#include "Line_MessagingAPI.h"
#include "secretsConfig.h"  

// ---------- helpers ----------
static String mapsLink(double lat, double lon) {
  return "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);
}

static String jsonEscape(const String &s) {
  String out; out.reserve(s.length() * 12 / 10 + 4);
  for (size_t i=0;i<s.length();++i) {
    char c = s[i];
    if (c=='"' || c=='\\') { out += '\\'; out += c; }
    else if (c=='\n') out += "\\n";
    else out += c;
  }
  return out;
}

// ---------- message builders ----------
String lineBuildText(const String &msg) {
  return String("{\"type\":\"text\",\"text\":\"") + jsonEscape(msg) + "\"}";
}

String lineBuildLocation(double lat, double lon, const String &title, const String &addr) {
  String j = "{";
  j += "\"type\":\"location\",";
  j += "\"title\":\""   + jsonEscape(title) + "\",";
  j += "\"address\":\"" + jsonEscape(addr)  + "\",";
  j += "\"latitude\":"  + String(lat, 6)    + ",";
  j += "\"longitude\":" + String(lon, 6);
  j += "}";
  return j;
}

// ---------- combine messages ----------
String lineBuildPushPayload(const String &to, const String &messageJsonArray) {
  String p = "{";
  p += "\"to\":\"" + to + "\",";
  p += "\"messages\":[";
  p += messageJsonArray;
  p += "]}";
  return p;
}

// ---------- HTTP sender ----------
bool linePush(const String &payload) {
  HTTPClient http;
  http.begin("https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + LINE_TOKEN);

  int code = http.POST(payload);
  Serial.printf("[LINE Push] HTTP %d\n", code);
  if (code < 0) {
    Serial.println("[LINE Push] " + http.errorToString(code));
    http.end();
    return false;
  }
  String resp = http.getString();
  Serial.println(resp);
  http.end();
  return (code >= 200 && code < 300);
}

// ---------- convenience ----------
bool lineSendText(const String &to, const String &text) {
  String payload = lineBuildPushPayload(to, lineBuildText(text));
  return linePush(payload);
}

bool lineSendLocation(const String &to, double lat, double lon, const String &title, const String &addr) {
  String payload = lineBuildPushPayload(to, lineBuildLocation(lat, lon, title, addr));
  return linePush(payload);
}

// ---------- main function (โดนชน + พิกัด) ----------
bool lineSendCrash(double lat, double lon) {
  String text = String("🚨 เกิดเหตุฉุกเฉิน!\nพิกัด: ") + String(lat,6) + "," + String(lon,6) + "\n" + mapsLink(lat, lon);
  String msgs = lineBuildText(text) + "," + lineBuildLocation(lat, lon);
  String payload = lineBuildPushPayload(LINE_USER, msgs);
  return linePush(payload);
}