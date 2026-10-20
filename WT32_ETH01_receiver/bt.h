#ifndef RECEIVER_BT_H
#define RECEIVER_BT_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"
#include "settings.h"

/* =============================================================
 * 接收機藍牙 Server (三擇一, 見 gSet.btMode)
 *   0 = BLE  (NimBLE) GATT 伺服器: 供 ESP32_D1_mini_rc 以 JSON 傳收
 *   1 = 傳統藍牙 (Classic BT / SPP): 供 ESP32_D1_mini_rc_BT 以 16-byte 協定
 *   2 = 關閉 (不啟動任何藍牙堆疊)
 * -------------------------------------------------------------
 * BLE Service (BLE_SVC_UUID):
 *   - CMD 特徵 (BLE_CMD_UUID): RC 寫入命令 (write / write-no-response)
 *   - TEL 特徵 (BLE_TEL_UUID): 接收機 notify 遙測 / ack 給 RC
 * 與 WiFi WebSocket 併存, 兩者都能收發命令 / 遙測。
 * 兩種藍牙無法同時啟動 (ESP32 只有一個主機堆疊), 以 btSetMode() 切換。
 * ============================================================= */

#include "spp.h"   /* 傳統藍牙 SPP server (含 BluetoothSerial) */

static NimBLEServer*         gBtServer   = nullptr;
static NimBLECharacteristic* gBtCmdChr   = nullptr;
static NimBLECharacteristic* gBtTelChr   = nullptr;
static bool     gBtConnected = false;
static uint16_t gBtConnHandle = 0xFFFF;
static uint32_t gBtConnCount = 0;
static uint32_t gBtRxLines   = 0;

/* 是否啟動中 (供 /api/bt 顯示) */
static inline bool btLinked() {
  if (gSet.btMode == BT_MODE_SPP) return gSppLinked;
  if (gSet.btMode == BT_MODE_BLE) return gBtConnected;
  return false;
}
static inline uint32_t btRxLines() {
  if (gSet.btMode == BT_MODE_SPP) return gSppRxLines;
  if (gSet.btMode == BT_MODE_BLE) return gBtRxLines;
  return 0;
}
static inline uint32_t btConnCount() {
  if (gSet.btMode == BT_MODE_SPP) return gSppConns;
  if (gSet.btMode == BT_MODE_BLE) return gBtConnCount;
  return 0;
}

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

/* ---------- BLE (NimBLE) 啟動 ---------- */
static inline bool bleBegin() {
  if (!NimBLEDevice::init(btName().c_str())) {
    Serial.println("[BLE] init 失敗 (記憶體不足?), 藍牙停用");
    return false;
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
  return true;
}

static inline void bleShutdown() {
  if (gBtServer && gBtConnected && gBtConnHandle != 0xFFFF) {
    gBtServer->disconnect(gBtConnHandle);
  }
  NimBLEDevice::deinit(true);     /* 釋放堆疊, 讓傳統藍牙能啟動 */
  gBtServer = nullptr;
  gBtCmdChr = nullptr;
  gBtTelChr = nullptr;
  gBtConnected = false;
  gBtConnHandle = 0xFFFF;
  Serial.println("[BLE] 已關閉");
}

/* ---------- 依 gSet.btMode 啟動對應的藍牙 Server ---------- */
/* 關閉目前模式的堆疊 (mode 為切換前的舊模式) */
static inline void btStopActive(uint8_t mode) {
  if (mode == BT_MODE_SPP) sppEnd();
  else if (mode == BT_MODE_BLE) bleShutdown();
}
/* 啟動指定模式 (BT_MODE_OFF 不做任何事) */
static inline bool btStartMode(uint8_t mode) {
  if (mode == BT_MODE_SPP) return sppBegin();
  if (mode == BT_MODE_BLE) return bleBegin();
  Serial.println("[BT] 藍牙 Server 已關閉 (可由網頁啟用)");
  return true;
}

static inline void btBegin() {
  btStartMode(gSet.btMode);
}

/* 切換藍牙通道 (0=BLE, 1=SPP, 2=關閉); 失敗回 false 並還原原模式 */
static bool btSetMode(uint8_t mode) {
  if (mode > BT_MODE_OFF) return false;
  if (mode == gSet.btMode) return true;
  uint8_t prev = gSet.btMode;

  btStopActive(prev);
  gSet.btMode = mode;
  if (!btStartMode(mode)) {        /* 失敗: 還原 */
    btStopActive(mode);
    gSet.btMode = prev;
    btStartMode(prev);
    return false;
  }
  settingsSaveBtMode();
  return true;
}

/* 傳送一行 JSON (遙測 / ack) 給已連線的 RC */
static inline void btSend(const String& line) {
  if (gSet.btMode != BT_MODE_BLE) return;  /* SPP 由 sppLoop() 送 16-byte; 關閉則略過 */
  if (!gBtConnected || !gBtTelChr || line.length() == 0) return;
  gBtTelChr->setValue((const uint8_t*)line.c_str(), line.length());
  gBtTelChr->notify();
}

static inline void btDisconnectClient() {
  if (gSet.btMode == BT_MODE_SPP) { sppDisconnect(); return; }
  if (gSet.btMode != BT_MODE_BLE) return;
  if (gBtServer && gBtConnected && gBtConnHandle != 0xFFFF) {
    gBtServer->disconnect(gBtConnHandle);
  }
}

/* NimBLE 為事件驅動; 傳統藍牙需輪詢收送 */
static inline void btLoop() {
  if (gSet.btMode == BT_MODE_SPP) sppLoop();
}

#endif
