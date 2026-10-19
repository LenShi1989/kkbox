#ifndef RC_LINK_H
#define RC_LINK_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "settings.h"
#include "telem.h"
#include "bt.h"

/* 本檔需在 .ino 內、localWs (AsyncWebSocket) 定義之後 include */

static WebSocketsClient linkWs;

/* 飛控板最後一筆回應 (ack): 供 /api/status 供網頁顯示/輪詢,
 * 即使瀏覽器 WebSocket 中斷 (改用 REST 送命令) 也能確認命令結果 */
static String   gLastAck   = "";
static uint32_t gLastAckMs = 0;

static void linkRelay(const String& msg) {
  localWs.textAll(msg);
}

/* 目前通道是否已與接收機建立連線 (依 gSet.channel 切換 WiFi / 藍牙) */
static inline bool linkUp() {
  return gSet.channel == 1 ? gBtUp : gLinkUp;
}

static void linkParse(const String& msg) {
  JsonDocument doc;
  if (deserializeJson(doc, msg)) return;
  if (!doc["gyro"].is<JsonObject>()) return;

  JsonObject g = doc["gyro"];
  JsonObject s = doc["set"];
  gTelem.roll  = g["roll"]  | 0.0f;
  gTelem.pitch = g["pitch"] | 0.0f;
  gTelem.yaw   = g["yaw"]   | 0.0f;
  gTelem.thr   = s["thr"]   | (long)1000;
  gTelem.armed = (doc["armed"] | 0) != 0;

  JsonArray m = doc["motor"];
  int i = 0;
  for (JsonVariant v : m) { if (i < 4) gTelem.motor[i++] = v.as<int>(); }

  gTelemetry   = msg;
  gTelemetryMs = millis();
  gTelem.ms    = gTelemetryMs;
  gTelem.valid = true;
}

/* 藍牙 (BLE) 收到一行: 與 WiFi WebSocket 收到文字相同處理 */
static void btOnLine(const String& msg) {
  if (msg.indexOf("\"ack\"") >= 0) { gLastAck = msg; gLastAckMs = millis(); }
  linkRelay(msg);
  if (msg.indexOf("\"gyro\"") >= 0) linkParse(msg);
}

static void onLinkEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      gLinkUp = true;
      Serial.println("[LINK] 已連線接收機");
      break;
    case WStype_DISCONNECTED:
      gLinkUp = false;
      Serial.println("[LINK] 已斷線");
      break;
    case WStype_TEXT: {
      String msg((char*)payload);
      if (length) msg = msg.substring(0, length);
      if (msg.indexOf("\"ack\"") >= 0) { gLastAck = msg; gLastAckMs = millis(); }
      linkRelay(msg);
      if (msg.indexOf("\"gyro\"") >= 0) linkParse(msg);
      break;
    }
    default:
      break;
  }
}

static inline void linkBegin() {
  String url = "/ws";
  if (gSet.rxToken.length()) url += "?token=" + gSet.rxToken;
  linkWs.begin(gSet.rxHost.c_str(), gSet.rxPort, url.c_str());
  linkWs.onEvent(onLinkEvent);
  linkWs.setReconnectInterval(3000);
  Serial.printf("[LINK] 目標 %s:%u%s\n", gSet.rxHost.c_str(), gSet.rxPort,
                gSet.rxToken.length() ? " (token)" : "");
}

static inline void linkLoop() {
  linkWs.loop();
}

/* 傳送控制命令 (JSON 字串) 給接收機 -> 飛控板
 * 依通道切換: 藍牙 (BLE) 或 WiFi (WebSocket) */
static inline void linkSend(String json) {
  if (gSet.channel == 1) { btSend(json); return; }
  if (gLinkUp) linkWs.sendTXT(json);
}

#endif
