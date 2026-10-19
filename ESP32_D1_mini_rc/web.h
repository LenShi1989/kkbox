#ifndef RC_WEB_H
#define RC_WEB_H

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
#include <ElegantOTA.h>
#include "config.h"
#include "settings.h"
#include "telem.h"
#include "link.h"

/* 本檔需在 .ino 內、server / localWs 定義之後 include */

/* ================= Session (登入權杖) ================= */
struct Session { String token; uint32_t born; };
static Session  gSessions[SESSION_MAX];
static uint32_t gRebootAt = 0;

static String cookieGet(AsyncWebServerRequest* r, const char* name) {
  if (!r->hasHeader("Cookie")) return "";
  String c = r->getHeader("Cookie")->value();
  String pat = String(name) + "=";
  int i = c.indexOf(pat);
  if (i < 0) return "";
  i += pat.length();
  int j = c.indexOf(';', i);
  return (j < 0) ? c.substring(i) : c.substring(i, j);
}
static void sessionAdd(const String& t) {
  for (int i = 0; i < SESSION_MAX; i++)
    if (gSessions[i].token.length() == 0) { gSessions[i] = {t, millis()}; return; }
  gSessions[0] = {t, millis()};
}
static void sessionDrop(const String& t) {
  for (int i = 0; i < SESSION_MAX; i++) if (gSessions[i].token == t) gSessions[i].token = "";
}
static bool tokenValid(const String& t) {
  if (t.length() == 0) return false;
  for (int i = 0; i < SESSION_MAX; i++) if (gSessions[i].token == t) return true;
  return false;
}
static bool isAuthed(AsyncWebServerRequest* r) {
  if (r->hasHeader("X-Auth") && tokenValid(r->getHeader("X-Auth")->value())) return true;
  if (r->hasParam("token") && tokenValid(r->getParam("token")->value())) return true;
  return tokenValid(cookieGet(r, "sess"));
}

/* ================= JSON ================= */
static String jsonEsc(const String& s) {
  String o;
  o.reserve(s.length() + 8);
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

static String buildFsJson() {
  String s = "{\"total\":" + String(SPIFFS.totalBytes());
  s += ",\"used\":" + String(SPIFFS.usedBytes()) + ",\"files\":[";
  File root = SPIFFS.open("/");
  File f = root.openNextFile();
  bool first = true;
  while (f) {
    if (!first) s += ',';
    first = false;
    String n = f.name();
    if (!n.startsWith("/")) n = "/" + n;
    s += "{\"name\":\"" + n + "\",\"size\":" + String((uint32_t)f.size()) + "}";
    f = root.openNextFile();
  }
  s += "]}";
  return s;
}

static String buildStatusJson() {
  String s = "{\"fw\":\"" FW_NAME "\",\"ver\":\"" FW_VERSION "\"";
  s += ",\"uptime\":" + String(millis() / 1000);
  s += ",\"heap\":" + String(ESP.getFreeHeap());

  bool wc = (WiFi.status() == WL_CONNECTED);
  s += ",\"wifi\":{\"connected\":" + String(wc ? 1 : 0);
  s += ",\"ssid\":\"" + (wc ? WiFi.SSID() : gSet.ssid) + "\"";
  s += ",\"ip\":\"" + (wc ? WiFi.localIP().toString() : String("-")) + "\"";
  s += ",\"rssi\":" + String(wc ? WiFi.RSSI() : 0) + "}";

  wifi_mode_t m = WiFi.getMode();
  bool ap = (m == WIFI_AP || m == WIFI_AP_STA);
  s += ",\"ap\":{\"active\":" + String(ap ? 1 : 0);
  s += ",\"ssid\":\"" + apSsid() + "\",\"ip\":\"" + (ap ? WiFi.softAPIP().toString() : String("-")) + "\"}";

  s += ",\"link\":{\"up\":" + String(linkUp() ? 1 : 0);
  s += ",\"host\":\"" + gSet.rxHost + "\",\"port\":" + String(gSet.rxPort);
  s += ",\"age\":" + String(gTelemetryMs ? (millis() - gTelemetryMs) : 999999) + "}";
  s += ",\"chan\":" + String(gSet.channel ? 1 : 0);
  s += ",\"wifiUp\":" + String(gLinkUp ? 1 : 0);
  s += ",\"btUp\":" + String(gBtUp ? 1 : 0);
  s += ",\"kkAck\":\"" + jsonEsc(gLastAck) + "\"";
  s += ",\"oled\":" + String(gOledOk ? 1 : 0) + "}";
  return s;
}

/* ================= 輔助 ================= */
static void sendJson(AsyncWebServerRequest* r, const String& body, int code = 200) {
  r->send(code, "application/json", body);
}
static void sendOk(AsyncWebServerRequest* r) { sendJson(r, "{\"ok\":true}"); }
static void sendErr(AsyncWebServerRequest* r, const char* msg, int code = 400) {
  sendJson(r, String("{\"err\":\"") + msg + "\"}", code);
}
static void requireAuth(AsyncWebServerRequest* r) {
  if (!isAuthed(r)) sendErr(r, "unauthorized", 401);
}
static String arg(AsyncWebServerRequest* r, const char* name, const char* def = "") {
  if (r->hasParam(name, true)) return r->getParam(name, true)->value();
  if (r->hasParam(name))       return r->getParam(name)->value();
  return String(def);
}

/* ================= WebSocket (本機 UI) ================= */
static void onLocalWs(AsyncWebSocket* s, AsyncWebSocketClient* c,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    c->printf("{\"hello\":\"rc\",\"ver\":\"" FW_VERSION "\"}");
    if (gTelemetryMs) c->text(gTelemetry);
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg = String((char*)data).substring(0, len);
      if (msg.startsWith("{")) {
        if (linkUp()) linkSend(msg);   /* 轉送接收機 -> 飛控板 */
        else c->text("{\"err\":\"link down\"}");   /* 斷線時明確回報, 不再靜默丟棄 */
      }
    }
  }
}

/* ================= 路由 ================= */
static void webSetup() {
  localWs.onEvent(onLocalWs);
  localWs.handleHandshake([](AsyncWebServerRequest* r) -> bool {
    if (r->hasParam("token") && tokenValid(r->getParam("token")->value())) return true;
    return tokenValid(cookieGet(r, "sess"));
  });
  server.addHandler(&localWs);

  server.on("/api/login", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (arg(r, "user") == gSet.user && arg(r, "pass") == gSet.pass) {
      String t = randomHex(24);
      sessionAdd(t);
      AsyncWebServerResponse* resp = r->beginResponse(200, "application/json",
        "{\"ok\":true,\"token\":\"" + t + "\"}");
      resp->addHeader("Set-Cookie", "sess=" + t + "; Path=/; HttpOnly");
      r->send(resp);
    } else sendErr(r, "bad credentials", 401);
  });
  server.on("/api/logout", HTTP_POST, [](AsyncWebServerRequest* r) {
    sessionDrop(cookieGet(r, "sess"));
    AsyncWebServerResponse* resp = r->beginResponse(200, "application/json", "{\"ok\":true}");
    resp->addHeader("Set-Cookie", "sess=; Path=/; Max-Age=0");
    r->send(resp);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, buildStatusJson());
  });
  server.on("/api/telemetry", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, gTelemetry);
  });
  server.on("/api/command", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String cmd = arg(r, "cmd");
    if (!cmd.length() || cmd[0] != '{') { sendErr(r, "bad command"); return; }
    if (!linkUp()) { sendErr(r, "link down", 503); return; }
    linkSend(cmd); sendOk(r);
  });

  server.on("/api/fs", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, buildFsJson());
  });

  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    int n = WiFi.scanNetworks();
    String s = "[";
    for (int i = 0; i < n; i++) {
      if (i) s += ',';
      s += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
           ",\"enc\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? 1 : 0) + "}";
    }
    s += "]";
    WiFi.scanDelete();
    sendJson(r, s);
  });
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    if (arg(r, "action") == "clear") {
      gSet.ssid = ""; gSet.wpass = "";
      gPrefs.putString("ssid", ""); gPrefs.putString("wpass", "");
      WiFi.disconnect(true);
      sendJson(r, "{\"ok\":true}");
      gRebootAt = millis() + 5000;   /* 清除後重啟系統 */
      return;
    }
    gSet.ssid  = arg(r, "ssid");
    gSet.wpass = arg(r, "pass");
    WiFi.mode(WIFI_AP_STA);      /* 保持 SoftAP 運作 */
    WiFi.disconnect();
    delay(20);
    WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    /* 等待連線 (最多 15 秒); 成功才儲存並回報取得的 IP, 數秒後重啟系統 */
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      gPrefs.putString("ssid", gSet.ssid); gPrefs.putString("wpass", gSet.wpass);
      String ip = WiFi.localIP().toString();
      Serial.printf("[WiFi] 已連線 %s -> %s, 5 秒後重啟\n", gSet.ssid.c_str(), ip.c_str());
      sendJson(r, "{\"ok\":true,\"ip\":\"" + ip + "\"}");
      gRebootAt = millis() + 5000;
    } else {
      sendErr(r, "WiFi 連線失敗, 請確認 SSID / 密碼");
    }
  });

  /* 接收機連線設定 */
  server.on("/api/link", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String s = "{\"host\":\"" + gSet.rxHost + "\",\"port\":" + String(gSet.rxPort) +
               ",\"token\":\"" + gSet.rxToken + "\"}";
    sendJson(r, s);
  });
  server.on("/api/link", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    gSet.rxHost  = arg(r, "host", gSet.rxHost.c_str());
    gSet.rxPort  = (uint16_t)arg(r, "port", "80").toInt();
    gSet.rxToken = arg(r, "token", gSet.rxToken.c_str());
    gPrefs.putString("rxhost", gSet.rxHost);
    gPrefs.putUInt("rxport", gSet.rxPort);
    gPrefs.putString("rxtoken", gSet.rxToken);
    linkWs.disconnect();
    linkBegin();
    sendOk(r);
  });

  /* 藍牙 (BLE) 設定 */
  server.on("/api/bt", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String s = "{\"up\":" + String(gBtUp ? 1 : 0);
    s += ",\"scanning\":" + String(gBtScanning ? 1 : 0);
    s += ",\"chan\":" + String(gSet.channel ? 1 : 0);
    s += ",\"name\":\"" + jsonEsc(gSet.btName) + "\"";
    s += ",\"addr\":\"" + gSet.btAddr + "\"";
    s += ",\"rx\":" + String(gBtRxLines);
    s += ",\"err\":\"" + jsonEsc(gBtLastErr) + "\"";
    s += ",\"devs\":" + btScanJson() + "}";
    sendJson(r, s);
  });
  server.on("/api/bt", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String a = arg(r, "action");
    if (a == "scan") {
      btScanStart();
      sendOk(r);
    } else if (a == "stop") {
      btScanStop();
      sendOk(r);
    } else if (a == "connect") {
      String addr = arg(r, "addr");
      if (!addr.length()) { sendErr(r, "missing addr"); return; }
      btConnectAddr(addr, arg(r, "name"));
      sendOk(r);
    } else if (a == "disconnect") {
      btDisconnect();
      sendOk(r);
    } else if (a == "clear") {
      btClear();
      sendOk(r);
    } else if (a == "channel") {
      gSet.channel = (uint8_t)(arg(r, "ch", "0").toInt() ? 1 : 0);
      settingsSaveBt();
      btApplyRadio();
      if (gSet.channel == 1 && gSet.btAddr.length() && !gBtUp) {
        btConnectAddr(gSet.btAddr, gSet.btName);
      }
      sendOk(r);
    } else {
      sendErr(r, "bad action");
    }
  });

  /* 使用者設定 */
  server.on("/api/user", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, "{\"user\":\"" + gSet.user + "\"}");
  });
  server.on("/api/user", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    if (arg(r, "cur") != gSet.pass) { sendErr(r, "wrong password", 401); return; }
    String u = arg(r, "user"); if (u.length()) gSet.user = u;
    String p = arg(r, "pass"); if (p.length()) gSet.pass = p;
    gPrefs.putString("user", gSet.user);
    gPrefs.putString("pass", gSet.pass);
    sendOk(r);
  });

  ElegantOTA.begin(&server);

  /* no-store: 避免瀏覽器快取舊版網頁 (重燒 SPIFFS 後仍抓到舊 app.js) */
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("no-store");

  server.onNotFound([](AsyncWebServerRequest* r) {
    if (r->method() == HTTP_OPTIONS) { r->send(204); return; }
    sendErr(r, "not found", 404);
  });

  server.begin();
}

#endif
