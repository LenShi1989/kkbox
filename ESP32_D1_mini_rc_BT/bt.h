#ifndef RC_BT_BT_H
#define RC_BT_BT_H

/* =============================================================
 * 傳統藍牙 (Classic BT / SPP) 用戶端
 * -------------------------------------------------------------
 * - 開機自動連線 WT32 藍牙 Server (serverMac 優先, 否則 serverName)
 * - 斷線後每 BT_RETRY_MS 自動重連
 * - RX 採累積取最新 16 bytes, 具自我同步能力 (SPP 為串流無幀界)
 * ============================================================= */

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "esp_mac.h"        /* esp_read_mac() */
#include "config.h"
#include "settings.h"
#include "proto.h"

static BluetoothSerial SerialBT;

/* 用於 RX 串流累積 (SPP 無幀界, 以最新 16 bytes 為一幀) */
static uint8_t gRxAcc[32];
static uint8_t gRxLen = 0;

static uint32_t gBtLastRetry = 0;
static uint32_t gBtLastSend = 0;
static bool     gBtWasLinked = false;

/* 本機裝置名稱: 前綴 + MAC 後 4 碼 */
static String btLocalName() {
  uint8_t m[6] = {0};
  esp_read_mac(m, ESP_MAC_WIFI_STA);
  char s[32];
  snprintf(s, sizeof s, "%s%02X%02X", BT_NAME_PREFIX, m[4], m[5]);
  return String(s);
}

static void btBegin() {
  String name = btLocalName();
  if (BT_PIN[0]) SerialBT.setPin(BT_PIN, true);
  if (!SerialBT.begin(name.c_str())) {
    Serial.println("[BT] 啟動失敗 (記憶體不足?)");
    return;
  }
  Serial.printf("[BT] 本機名稱: %s\n", name.c_str());
  Serial.printf("[BT] 目標 Server: %s\n",
                gSet.serverMac.length() ? gSet.serverMac.c_str() : gSet.serverName.c_str());
}

/* 嘗試連線一次 (成功回 true) */
static bool btTryConnect() {
  if (gSet.serverMac.length()) {
    if (SerialBT.connect(gSet.serverMac.c_str())) {
      Serial.printf("[BT] 已連線 (MAC %s)\n", gSet.serverMac.c_str());
      return true;
    }
  }
  if (gSet.serverName.length()) {
    if (SerialBT.connect(gSet.serverName.c_str())) {
      Serial.printf("[BT] 已連線 (%s)\n", gSet.serverName.c_str());
      return true;
    }
  }
  return false;
}

/* 讀取 SPP 串流: 累積後取最新 16 bytes 作為一幀遙測 */
static void btPollRx() {
  while (SerialBT.available()) {
    int c = SerialBT.read();
    if (c < 0) break;
    if (gRxLen < sizeof gRxAcc) gRxAcc[gRxLen++] = (uint8_t)c;
  }
  if (gRxLen >= 16) {
    memcpy(gRx, gRxAcc + (gRxLen - 16), 16);
    gRxLen = 0;
    gRxNew = true;
    gRxCount++;
  }
}

static void btLoop() {
  bool linked = SerialBT.hasClient();
  if (linked != gBtLinked) {
    gBtLinked = linked;
    if (!linked) {
      gRxLen = 0;
      gRxNew = false;
      gBtLastRetry = millis();
      Serial.println("[BT] 連線中斷, 準備重連...");
    } else {
      Serial.println("[BT] 連線建立");
    }
    gBtWasLinked = linked;
  }

  if (!linked) {
    if (millis() - gBtLastRetry >= BT_RETRY_MS) {
      gBtLastRetry = millis();
      if (btTryConnect()) Serial.println("[BT] 連線成功");
    }
    return;
  }

  btPollRx();

  /* 每 BT_SEND_MS 傳送一次 16 bytes 遙控資料 */
  if (millis() - gBtLastSend >= BT_SEND_MS) {
    gBtLastSend = millis();
    protoPack();
    if (SerialBT.write(gTx, 16) == 16) gTxCount++;
  }
}

static void btDisconnect() {
  if (SerialBT.hasClient()) SerialBT.disconnect();
}

#endif
