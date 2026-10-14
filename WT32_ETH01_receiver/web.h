#ifndef RECEIVER_WEB_H
#define RECEIVER_WEB_H

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
#include <ElegantOTA.h>
#include "config.h"
#include "settings.h"
#include "kkbox.h"

/* ================= 全域伺服器物件 ================= */
static AsyncWebServer  server(WEB_PORT);
static AsyncWebSocket  ws("/ws");

/* ================= Session (登入權杖) ================= */
struct Session { String token; uint32_t born; };
static Session gSessions[SESSION_MAX];

static uint32_t gRebootAt = 0;   /* >0 表示排程重開機 */

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
  for (int i = 0; i < SESSION_MAX; i++) {
    if (gSessions[i].token.length() == 0) { gSessions[i] = {t, millis()}; return; }
  }
  gSessions[0] = {t, millis()};   /* 滿了就覆蓋最舊 */
}

static void sessionDrop(const String& t) {
  for (int i = 0; i < SESSION_MAX; i++)
    if (gSessions[i].token == t) gSessions[i].token = "";
}

static bool tokenValid(const String& t) {
  if (t.length() == 0) return false;
  if (t == gSet.deviceToken) return true;
  for (int i = 0; i < SESSION_MAX; i++)
    if (gSessions[i].token == t) return true;
  return false;
}

static bool isAuthed(AsyncWebServerRequest* r) {
  if (r->hasHeader("X-Auth") && tokenValid(r->getHeader("X-Auth")->value())) return true;
  if (r->hasParam("token") && tokenValid(r->getParam("token")->value())) return true;
  return tokenValid(cookieGet(r, "sess"));
}

/* ================= JSON 組裝 ================= */
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

  /* RJ45 */
  bool link = ETH.linkUp();
  s += ",\"eth\":{\"link\":" + String(link ? 1 : 0);
  s += ",\"ip\":\"" + (link ? ETH.localIP().toString() : String("-")) + "\"";
  s += ",\"speed\":" + String(link ? ETH.linkSpeed() : 0);
  s += ",\"dhcp\":" + String(gSet.dhcp ? 1 : 0) + "}";

  /* WiFi */
  bool wc = (WiFi.status() == WL_CONNECTED);
  s += ",\"wifi\":{\"connected\":" + String(wc ? 1 : 0);
  s += ",\"ssid\":\"" + (wc ? WiFi.SSID() : gSet.ssid) + "\"";
  s += ",\"ip\":\"" + (wc ? WiFi.localIP().toString() : String("-")) + "\"";
  s += ",\"rssi\":" + String(wc ? WiFi.RSSI() : 0) + "}";

  /* SoftAP */
  wifi_mode_t m = WiFi.getMode();
  bool ap = (m == WIFI_AP || m == WIFI_AP_STA);
  s += ",\"ap\":{\"active\":" + String(ap ? 1 : 0);
  s += ",\"ssid\":\"" AP_SSID "\",\"ip\":\"" + (ap ? WiFi.softAPIP().toString() : String("-")) + "\"}";

  s += ",\"kk\":{\"lines\":" + String(gKkLines);
  s += ",\"age\":" + String(gLastTelemetryMs ? (millis() - gLastTelemetryMs) : 999999) + "}";
  s += ",\"token\":\"" + gSet.deviceToken + "\"}";
  return s;
}

/* ================= 回應輔助 ================= */
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

/* ================= WebSocket 事件 ================= */
static void onWsEvent(AsyncWebSocket* s, AsyncWebSocketClient* c,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    c->printf("{\"hello\":\"receiver\",\"ver\":\"" FW_VERSION "\"}");
    if (gLastTelemetryMs) c->text(gLastTelemetry);
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg = String((char*)data).substring(0, len);
      if (msg.startsWith("{")) kkSend(msg);   /* 控制命令直通飛控板 */
    }
  }
}

/* ================= 設定精靈 / 路由 ================= */
static void webSetup() {
  ws.onEvent(onWsEvent);
  ws.handleHandshake([](AsyncWebServerRequest* r) -> bool {
    if (r->hasParam("token") && tokenValid(r->getParam("token")->value())) return true;
    return tokenValid(cookieGet(r, "sess"));
  });
  server.addHandler(&ws);

  /* ---- 登入 / 登出 ---- */
  server.on("/api/login", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (arg(r, "user") == gSet.user && arg(r, "pass") == gSet.pass) {
      String t = randomHex(24);
      sessionAdd(t);
      AsyncWebServerResponse* resp = r->beginResponse(200, "application/json",
        "{\"ok\":true,\"token\":\"" + t + "\"}");
      resp->addHeader("Set-Cookie", "sess=" + t + "; Path=/; HttpOnly");
      r->send(resp);
    } else {
      sendErr(r, "bad credentials", 401);
    }
  });
  server.on("/api/logout", HTTP_POST, [](AsyncWebServerRequest* r) {
    sessionDrop(cookieGet(r, "sess"));
    AsyncWebServerResponse* resp = r->beginResponse(200, "application/json", "{\"ok\":true}");
    resp->addHeader("Set-Cookie", "sess=; Path=/; Max-Age=0");
    r->send(resp);
  });

  /* ---- 狀態 / 遙測 ---- */
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, buildStatusJson());
  });
  server.on("/api/telemetry", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, gLastTelemetry);
  });

  /* ---- 轉送控制命令到飛控板 ---- */
  server.on("/api/command", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String cmd = arg(r, "cmd");
    if (cmd.length() && cmd[0] == '{') { kkSend(cmd); sendOk(r); }
    else sendErr(r, "bad command");
  });

  /* ---- 檔案系統 ---- */
  server.on("/api/fs", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, buildFsJson());
  });

  /* ---- WiFi 掃描 / 設定 ---- */
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
      settingsSaveWifi(); WiFi.disconnect(true);
      sendOk(r); return;
    }
    gSet.ssid  = arg(r, "ssid");
    gSet.wpass = arg(r, "pass");
    settingsSaveWifi();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    sendOk(r);
  });

  /* ---- RJ45 網路設定 ---- */
  server.on("/api/net", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String s = "{\"dhcp\":" + String(gSet.dhcp ? 1 : 0) +
               ",\"ip\":\"" + gSet.ip + "\",\"gw\":\"" + gSet.gw +
               "\",\"mask\":\"" + gSet.mask + "\"}";
    sendJson(r, s);
  });
  server.on("/api/net", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    gSet.dhcp = arg(r, "dhcp", "1") == "1";
    gSet.ip   = arg(r, "ip", gSet.ip.c_str());
    gSet.gw   = arg(r, "gw", gSet.gw.c_str());
    gSet.mask = arg(r, "mask", gSet.mask.c_str());
    settingsSaveNet();
    gRebootAt = millis() + 800;   /* 套用需重開機 */
    sendOk(r);
  });

  /* ---- PID ---- */
  server.on("/api/pid", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    char b[320];
    snprintf(b, sizeof b,
      "{\"roll\":{\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f},"
      "\"pitch\":{\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f},"
      "\"yaw\":{\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f}}",
      gSet.rollKp, gSet.rollKi, gSet.rollKd,
      gSet.pitchKp, gSet.pitchKi, gSet.pitchKd,
      gSet.yawKp, gSet.yawKi, gSet.yawKd);
    sendJson(r, b);
  });
  server.on("/api/pid", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String a = arg(r, "axis", "roll");
    float kp = arg(r, "kp", "0").toFloat();
    float ki = arg(r, "ki", "0").toFloat();
    float kd = arg(r, "kd", "0").toFloat();
    if (a == "roll")  { gSet.rollKp = kp; gSet.rollKi = ki; gSet.rollKd = kd; }
    if (a == "pitch") { gSet.pitchKp = kp; gSet.pitchKi = ki; gSet.pitchKd = kd; }
    if (a == "yaw")   { gSet.yawKp = kp; gSet.yawKi = ki; gSet.yawKd = kd; }
    settingsSavePid();
    char b[96];
    snprintf(b, sizeof b, "{\"cmd\":\"pid\",\"axis\":\"%s\",\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f}",
             a.c_str(), kp, ki, kd);
    kkSend(b);
    sendOk(r);
  });

  /* ---- 使用者設定 ---- */
  server.on("/api/user", HTTP_GET, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    sendJson(r, "{\"user\":\"" + gSet.user + "\",\"token\":\"" + gSet.deviceToken + "\"}");
  });
  server.on("/api/user", HTTP_POST, [](AsyncWebServerRequest* r) {
    requireAuth(r); if (r->getResponse()) return;
    String nu = arg(r, "user", gSet.user.c_str());
    String np = arg(r, "pass");
    if (arg(r, "cur") == gSet.pass) {
      if (arg(r, "newtoken") == "1") gSet.deviceToken = randomHex(16), gPrefs.putString("dtok", gSet.deviceToken);
      if (arg(r, "newuser").length()) gSet.user = arg(r, "user");
      if (arg(r, "newpass").length()) gSet.pass = arg(r, "pass");
      settingsSaveUser();
      ElegantOTA.setAuth(gSet.user.c_str(), gSet.pass.c_str());
      sendOk(r);
    } else {
      sendErr(r, "wrong password", 401);
    }
  });

  /* ---- OTA (ElegantOTA) ---- */
  ElegantOTA.begin(&server, gSet.user.c_str(), gSet.pass.c_str());

  /* ---- 靜態檔案 (SPIFFS) ---- */
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest* r) {
    if (r->method() == HTTP_OPTIONS) { r->send(204); return; }
    sendErr(r, "not found", 404);
  });

  server.begin();
}

#endif
