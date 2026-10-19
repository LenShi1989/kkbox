#ifndef RECEIVER_BT_H
#define RECEIVER_BT_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"
#include "settings.h"

/* =============================================================
 * 接收機 BLE (NimBLE) GATT 伺服器 (peripheral)
 * -------------------------------------------------------------
 * 提供一個 Service (BLE_SVC_UUID):
 *   - CMD 特徵 (BLE_CMD_UUID): RC 寫入命令 (write / write-no-response)
 *   - TEL 特徵 (BLE_TEL_UUID): 接收機 notify 遙測 / ack 給 RC
 * 與 WiFi WebSocket 併存, 兩者都能收發命令 / 遙測。
 * ============================================================= */

static NimBLEServer*         gBtServer   = nullptr;
static NimBLECharacteristic* gBtCmdChr   = nullptr;
static NimBLECharacteristic* gBtTelChr   = nullptr;
static bool     gBtConnected = false;
static uint16_t gBtConnHandle = 0xFFFF;
static uint32_t gBtConnCount = 0;
static uint32_t gBtRxLines   = 0;

/* 由 web.h 定義: 收到 RC 一行命令 (JSON) */
static void btOnLine(const String& msg);

class BtServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
    gBtConnected  = true;
    gBtConnHandle = info.getConnHandle();
    gBtConnCount++;
    Serial.println("[BLE] 客戶端已連線");
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int reason) override {
    gBtConnected = false;
    gBtConnHandle = 0xFFFF;
    Serial.println("[BLE] 客戶端已斷線, 重新廣播");
    NimBLEDevice::startAdvertising();
  }
};

class BtCmdCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& info) override {
    NimBLEAttValue v = c->getValue();
    if (v.length() == 0) return;
    String msg((const char*)v.data(), v.length());
    msg.trim();
    if (!msg.length()) return;
    gBtRxLines++;
    btOnLine(msg);
  }
};

static inline void btBegin() {
  if (!NimBLEDevice::init(btName().c_str())) {
    Serial.println("[BLE] init 失敗 (記憶體不足?), 藍牙停用");
    return;
  }
  NimBLEDevice::setPowerLevel(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(517);              /* 允許較長遙測 / 命令 */
  gBtServer = NimBLEDevice::createServer();
  gBtServer->setCallbacks(new BtServerCB());

  NimBLEService* svc = gBtServer->createService(BLE_SVC_UUID);
  gBtCmdChr = svc->createCharacteristic(BLE_CMD_UUID,
                NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  gBtCmdChr->setCallbacks(new BtCmdCB());
  gBtTelChr = svc->createCharacteristic(BLE_TEL_UUID,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  gBtServer->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->enableScanResponse(true);
  adv->setName(btName().c_str());
  adv->addServiceUUID(BLE_SVC_UUID);
  if (adv->start()) Serial.printf("[BLE] 廣播已啟動: %s\n", btName().c_str());
  else              Serial.println("[BLE] 廣播啟動失敗");
}

/* 傳送一行 JSON (遙測 / ack) 給已連線的 RC */
static inline void btSend(const String& line) {
  if (!gBtConnected || !gBtTelChr || line.length() == 0) return;
  gBtTelChr->setValue((const uint8_t*)line.c_str(), line.length());
  gBtTelChr->notify();
}

static inline void btDisconnectClient() {
  if (gBtServer && gBtConnected && gBtConnHandle != 0xFFFF) {
    gBtServer->disconnect(gBtConnHandle);
  }
}

/* NimBLE 為事件驅動, 無需輪詢; 保留掛勾以維持既有呼叫 */
static inline void btLoop() {}

#endif
