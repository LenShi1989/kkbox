#ifndef RECEIVER_SPP_H
#define RECEIVER_SPP_H

/* =============================================================
 * 接收機 傳統藍牙 (Classic BT / SPP) 伺服器
 * -------------------------------------------------------------
 * 供 ESP32_D1_mini_rc_BT (SPP client) 以 16-byte 二進位協定連線:
 *   - 接收: 16 bytes 遙控資料 -> 轉成 kkbox JSON 指令下傳
 *           (throttle 1000~2000 / setpoints ±SPP_MAX_SET deg/s)
 *   - 傳送: 每 SPP_TEL_MS 依最新遙測組一幀 16 bytes 遙測回給遙控器
 * 與 BLE (NimBLE) 互斥, 由 gSet.btMode 決定啟動哪一個 (見 bt.h)。
 * ============================================================= */

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "config.h"
#include "settings.h"
#include "kkbox.h"

static inline void kkSendLog(const String& s);   /* 由 web.h 定義 */

static BluetoothSerial SerialBT;

static bool     gSppLinked = false;
static uint32_t gSppConns = 0;        /* 累計連線次數 */
static uint32_t gSppRxLines = 0;      /* 累計收到的遙控封包數 */
static bool     gSppJustOpen = false; /* callback 設定, loop 處理 */
static bool     gSppJustClose = false;

static uint8_t  gSppAcc[32];          /* SPP 串流累積 (無幀界, 取最新 16 bytes) */
static uint8_t  gSppAccLen = 0;

static long     gSppThr = 1000;
static float    gSppRoll = 0, gSppPitch = 0, gSppYaw = 0;
static uint32_t gSppLastSp = 0, gSppLastThr = 0, gSppLastHb = 0, gSppLastTel = 0;
static bool     gSppSent = false;     /* 是否曾下傳過指令 (斷線才需要安全油門) */

/* ---------- 工具: 從 JSON 字串取數值 (無動態記憶體) ---------- */
static long sppNum(const char* s, const char* key, long dflt) {
  const char* p = strstr(s, key);
  if (!p) return dflt;
  p += strlen(key);
  char* end = nullptr;
  long v = strtol(p, &end, 10);
  return (end == p) ? dflt : v;
}
static inline long sppClamp(long v, long lo, long hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

/* ---------- 斷線/關閉時油門歸底 (安全) ---------- */
static void sppSafeThrottle() {
  if (!gSppSent) return;
  gSppSent = false;
  gSppThr = 1000;
  kkSendLog("{\"cmd\":\"throttle\",\"value\":1000}");
}

/* ---------- SPP 事件 (BT task 內僅設旗標, 實際處理在 loop) ---------- */
static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    gSppJustOpen = true;
  } else if (event == ESP_SPP_CLOSE_EVT) {
    gSppJustClose = true;
  }
}

/* ---------- 依最新遙測組 16 bytes 回傳給遙控器 ---------- */
static void sppSendTelemetry() {
  if (!gSppLinked) return;
  uint8_t f[16];
  memset(f, 0, sizeof f);

  const char* j = gLastTelemetry.c_str();

  /* [0..7] 馬達 1000~2000us -> 0~255 */
  const char* m = strstr(j, "\"motor\":[");
  if (m) {
    m += strlen("\"motor\":[");
    for (int i = 0; i < 8; i++) {
      char* end = nullptr;
      long v = strtol(m, &end, 10);
      if (end == m) break;
      long b = (v - 1000) * 255 / 1000;
      f[i] = (uint8_t)sppClamp(b, 0, 255);
      m = end;
      if (*m != ',') break;
      m++;
    }
  }

  /* [8] pitch, [9] roll, [10] yaw (陀螺角速率, deg/s, signed) */
  const char* g = strstr(j, "\"gyro\":{");
  if (g) {
    f[8]  = (uint8_t)(int8_t)sppClamp(sppNum(g, "\"pitch\":", 0), -127, 127);
    f[9]  = (uint8_t)(int8_t)sppClamp(sppNum(g, "\"roll\":", 0),  -127, 127);
    f[10] = (uint8_t)(int8_t)sppClamp(sppNum(g, "\"yaw\":", 0),   -127, 127);
  }

  /* [11] 溫度 / [12..13] 高度: kkbox 未提供 */
  f[14] = SPP_DRONE_ID;   /* 無人機編號 */
  f[15] = 1;              /* 1 = 有新資料 */

  SerialBT.write(f, sizeof f);
}

/* ---------- 搖桿 byte -> 角速率 (deg/s) ---------- */
static float sppAxis(uint8_t v) {
  int c = (int)v - 128;
  if (c >= -SPP_DEADBAND && c <= SPP_DEADBAND) return 0.0f;
  return (float)c * ((float)SPP_MAX_SET / 127.0f);
}

/* ---------- 16 bytes 遙控資料 -> kkbox 指令 ---------- */
static void sppHandleFrame(const uint8_t* t) {
  uint32_t now = millis();
  long thr = 1000 + (long)(((unsigned long)t[0] * 1000UL + 127UL) / 255UL);
  float roll  = sppAxis(t[3]);
  float pitch = sppAxis(t[2]);
  float yaw   = sppAxis(t[1]);

  bool thrCh = (thr != gSppThr);
  bool spCh  = (roll != gSppRoll || pitch != gSppPitch || yaw != gSppYaw);
  bool hb    = (now - gSppLastHb >= SPP_HB_MS);
  bool thrDue = (now - gSppLastThr >= SPP_THR_MS);
  bool spDue  = (now - gSppLastSp  >= SPP_CMD_MS);
  if (!hb && !((thrDue && thrCh) || (spDue && spCh))) return;

  char b[96];
  if (hb || (thrDue && thrCh)) {
    gSppLastThr = now;
    snprintf(b, sizeof b, "{\"cmd\":\"throttle\",\"value\":%ld}", thr);
    kkSendLog(b);
    gSppThr = thr;
  }
  if (hb || (spDue && spCh)) {
    gSppLastSp = now;
    snprintf(b, sizeof b, "{\"cmd\":\"setpoints\",\"roll\":%.0f,\"pitch\":%.0f,\"yaw\":%.0f}",
             roll, pitch, yaw);
    kkSendLog(b);
    gSppRoll = roll; gSppPitch = pitch; gSppYaw = yaw;
  }
  if (hb) gSppLastHb = now;
  gSppSent = true;
}

/* ---------- 啟動 / 關閉 ---------- */
static bool sppBegin() {
  if (SPP_PIN[0]) SerialBT.setPin(SPP_PIN, true);
  SerialBT.register_callback(sppCallback);
  if (!SerialBT.begin(SPP_NAME)) {
    Serial.println("[SPP] 啟動失敗 (記憶體不足?)");
    return false;
  }
  Serial.printf("[SPP] Server 已啟動: %s (等遙控器連線)\n", SPP_NAME);
  return true;
}

static void sppEnd() {
  sppSafeThrottle();
  gSppLinked = false;
  gSppAccLen = 0;
  SerialBT.end();
  Serial.println("[SPP] 已關閉");
}

static void sppDisconnect() {
  if (!gSppLinked) return;
  sppSafeThrottle();
  SerialBT.disconnect();
  gSppLinked = false;
  gSppAccLen = 0;
}

/* ---------- 主迴圈 ---------- */
static void sppLoop() {
  if (gSppJustOpen) {
    gSppJustOpen = false;
    gSppLinked = true;
    gSppConns++;
    gSppLastHb = millis();
    Serial.println("[SPP] 遙控器已連線");
    sppSendTelemetry();
  }
  if (gSppJustClose) {
    gSppJustClose = false;
    if (gSppLinked) Serial.println("[SPP] 遙控器斷線");
    gSppLinked = false;
    gSppAccLen = 0;
    sppSafeThrottle();
  }
  if (!gSppLinked) return;

  while (SerialBT.available()) {
    int c = SerialBT.read();
    if (c < 0) break;
    if (gSppAccLen < sizeof gSppAcc) gSppAcc[gSppAccLen++] = (uint8_t)c;
  }
  if (gSppAccLen >= 16) {
    sppHandleFrame(gSppAcc + (gSppAccLen - 16));
    gSppAccLen = 0;
    gSppRxLines++;
  }

  if (millis() - gSppLastTel >= SPP_TEL_MS) {
    gSppLastTel = millis();
    sppSendTelemetry();
  }
}

#endif
