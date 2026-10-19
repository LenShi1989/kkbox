#ifndef RC_BT_H
#define RC_BT_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"
#include "settings.h"

/* =============================================================
 * 遙控器 BLE (NimBLE) 用戶端 (central)
 * -------------------------------------------------------------
 * 掃描 / 連線 / 配對 WT32-ETH01 接收機 (BLE_SVC_UUID),
 * 訂閱 TEL 特徵接收遙測 / ack, 以 CMD 特徵寫入控制命令。
 * 與 WiFi WebSocket 併存, 由 gSet.channel 決定目前通道。
 * ============================================================= */

static bool     gBtUp        = false;
static bool     gBtScanning  = false;
static uint32_t gBtRxLines   = 0;
static String   gBtLastErr   = "";

struct BtDev { NimBLEAddress addr; String name; int rssi; };
static BtDev gBtDevs[BT_SCAN_MAX];
static int   gBtDevCount = 0;

static NimBLEAddress gBtConnAddr;
static bool          gBtDoConnect  = false;
static String        gBtTargetAddr = "";
static String        gBtTargetName = "";

static NimBLEClient*                gBtClient = nullptr;
static NimBLERemoteCharacteristic*  gBtCmdChr = nullptr;
static NimBLERemoteCharacteristic*  gBtTelChr = nullptr;

/* 由 link.h 定義: 收到接收機一行 JSON */
static void btOnLine(const String& msg);

static String btEsc(const String& s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

/* ---------- 掃描回呼 ---------- */
class BtScanCB : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    if (!dev) return;
    std::string nms = dev->haveName() ? dev->getName() : std::string("");
    String nm = String(nms.c_str());
    bool match = nm.startsWith("KKBOX-RX-") ||
                 dev->isAdvertisingService(NimBLEUUID(BLE_SVC_UUID));
    if (!match) return;
    String a = String(dev->getAddress().toString().c_str());

    if (gBtTargetAddr.length() && a == gBtTargetAddr) {
      gBtConnAddr  = dev->getAddress();
      gBtDoConnect = true;
    }

    for (int i = 0; i < gBtDevCount; i++) {
      if (String(gBtDevs[i].addr.toString().c_str()) == a) {
        gBtDevs[i].rssi = dev->getRSSI();
        return;
      }
    }
    if (gBtDevCount >= BT_SCAN_MAX) return;
    gBtDevs[gBtDevCount].addr = dev->getAddress();
    gBtDevs[gBtDevCount].name = nm.length() ? nm : a;
    gBtDevs[gBtDevCount].rssi = dev->getRSSI();
    gBtDevCount++;
  }
};
static BtScanCB btScanCB;

static void btScanEndCB(NimBLEScanResults results) { gBtScanning = false; }

/* ---------- 通知 (遙測 / ack) 回呼 ---------- */
static void btNotifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data,
                       size_t len, bool isNotify) {
  if (!data || len == 0) return;
  String msg((char*)data);
  msg = msg.substring(0, len);
  gBtRxLines++;
  btOnLine(msg);
}

/* ---------- 連線回呼 ---------- */
class BtClientCB : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* c) override {
    gBtUp = false;
    gBtCmdChr = nullptr;
    gBtTelChr = nullptr;
    Serial.println("[BLE] 已斷線");
  }
};
static BtClientCB btClientCB;

/* ---------- 掃描控制 ---------- */
static inline void btScanStart() {
  gBtDevCount = 0;
  gBtScanning = true;
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->clearResults();
  scan->start(5, btScanEndCB, false);   /* 5 秒, 非阻塞 */
}
static inline void btScanStop() {
  NimBLEDevice::getScan()->stop();
  gBtScanning = false;
}

/* ---------- 連線 ---------- */
static bool btConnectNow(const NimBLEAddress& addr) {
  NimBLEScan* scan = NimBLEDevice::getScan();
  if (gBtScanning) { scan->stop(); gBtScanning = false; }

  if (!gBtClient) {
    gBtClient = NimBLEDevice::createClient();
    gBtClient->setClientCallbacks(&btClientCB, false);
    gBtClient->setConnectTimeout(5);
    gBtClient->setConnectionParams(12, 12, 0, 51);
  }
  gBtLastErr = "";
  if (!gBtClient->connect(addr)) { gBtLastErr = "連線失敗"; return false; }

  NimBLERemoteService* svc = gBtClient->getService(BLE_SVC_UUID);
  if (!svc) { gBtLastErr = "找不到服務"; gBtClient->disconnect(); return false; }

  gBtCmdChr = svc->getCharacteristic(BLE_CMD_UUID);
  gBtTelChr = svc->getCharacteristic(BLE_TEL_UUID);
  if (!gBtCmdChr || !gBtTelChr) {
    gBtLastErr = "找不到特徵"; gBtClient->disconnect(); return false;
  }
  if (gBtTelChr->canNotify()) gBtTelChr->subscribe(true, btNotifyCB);

  gBtUp = true;
  gSet.btAddr = String(addr.toString().c_str());
  if (gBtTargetName.length()) gSet.btName = gBtTargetName;
  settingsSaveBt();
  Serial.printf("[BLE] 已連線 %s\n", gSet.btAddr.c_str());
  return true;
}

/* 要求連線到指定位址 (清單內直接連, 否則先掃描尋找) */
static inline void btConnectAddr(const String& addr, const String& name) {
  gBtTargetAddr = addr;
  gBtTargetName = name;
  gBtDoConnect  = false;
  for (int i = 0; i < gBtDevCount; i++) {
    if (String(gBtDevs[i].addr.toString().c_str()) == addr) {
      gBtConnAddr  = gBtDevs[i].addr;
      gBtDoConnect = true;
      return;
    }
  }
  if (!gBtScanning) btScanStart();
}

static inline void btDisconnect() {
  gBtDoConnect  = false;
  gBtTargetAddr = "";
  if (gBtClient && gBtClient->isConnected()) gBtClient->disconnect();
  gBtUp = false;
}

/* 清除配對: 斷線 + 刪除用戶端 + 清除已儲存位址 */
static inline void btClear() {
  btDisconnect();
  if (gBtClient) { NimBLEDevice::deleteClient(gBtClient); gBtClient = nullptr; }
  gBtCmdChr = nullptr;
  gBtTelChr = nullptr;
  gSet.btAddr = "";
  gSet.btName = "";
  settingsSaveBt();
}

/* 送出控制命令 (RC -> 接收機) */
static inline void btSend(const String& cmd) {
  if (!gBtUp || !gBtCmdChr) return;
  gBtCmdChr->writeValue(cmd.c_str(), true);
}

static String btScanJson() {
  String s = "[";
  for (int i = 0; i < gBtDevCount; i++) {
    if (i) s += ',';
    s += "{\"addr\":\"" + String(gBtDevs[i].addr.toString().c_str()) + "\"";
    s += ",\"name\":\"" + btEsc(gBtDevs[i].name) + "\"";
    s += ",\"rssi\":" + String(gBtDevs[i].rssi) + "}";
  }
  s += "]";
  return s;
}

static inline void btBegin() {
  NimBLEDevice::init(btDevName().c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(517);
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(&btScanCB, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  Serial.printf("[BLE] 初始化完成: %s\n", btDevName().c_str());
  /* 通道為藍牙且已配對過: 開機自動重連 */
  if (gSet.channel == 1 && gSet.btAddr.length()) {
    btConnectAddr(gSet.btAddr, gSet.btName);
  }
}

static inline void btLoop() {
  /* 找到目標位址且尚未連線 -> 進行 (阻塞式) 連線 */
  if (gBtDoConnect && !gBtUp) {
    gBtDoConnect = false;
    btConnectNow(gBtConnAddr);
  }
  /* 通道為藍牙但未連線: 每 10 秒自動重試 */
  static uint32_t lastTry = 0;
  if (gSet.channel == 1 && !gBtUp && !gBtScanning &&
      gSet.btAddr.length() && millis() - lastTry > 10000) {
    lastTry = millis();
    btConnectAddr(gSet.btAddr, gSet.btName);
  }
}

#endif
