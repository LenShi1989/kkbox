#ifndef RC_CONFIG_H
#define RC_CONFIG_H

/* =============================================================
 * ESP32 D1 mini 遙控器 (RC) 設定
 * -------------------------------------------------------------
 * 版本 (Version): 2.0.0   (見 FW_VERSION)
 * 名稱 (Name):    ESP32 D1 mini RC
 * -------------------------------------------------------------
 * 功能:
 *   - WiFi 連線, 以 WebSocket 用戶端連線至 WT32-ETH01 接收機
 *   - 藍牙 BLE (NimBLE) 用戶端: 掃描 / 連線配對接收機, WiFi <-> BLE 通道切換
 *   - SSD1306 128x64 OLED 顯示遙測
 *   - SPIFFS 網頁介面 (ESPAsyncWebServer) + WebSocket
 *   - 模擬搖桿 / WiFi / 藍牙 / 使用者 / OTA 設定
 * ============================================================= */

#include <Arduino.h>

#define FW_NAME "ESP32 D1 mini RC"
#define FW_VERSION "2.0.0"

/* ---------- OLED (SSD1306 I2C) ---------- */
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64

/* ---------- Web / AP ---------- */
#define WEB_PORT 80
/* SoftAP 一律啟動, 與 STA 共存; SSID = 前綴 + MAC 後 4 碼 (見 settings.h apSsid()) */
#define AP_SSID_PREFIX "KKBOX-RC-"
#define AP_PASS "12345678"
/* RC 的 SoftAP 使用 192.168.5.x, 與接收機 (192.168.4.x) 區隔, 避免 STA 連上對方 AP 時網段衝突 */
#define AP_IP_ADDR "192.168.10.1"
#define SESSION_MAX 8

/* ---------- 預設帳密 ---------- */
#define DEF_USER "admin"
#define DEF_PASS "admin"

/* ---------- 預設接收機連線 ---------- */
#define DEF_RX_PORT 80

/* ---------- 藍牙 BLE (NimBLE) ---------- */
/* 連線通道: 0 = WiFi (WebSocket), 1 = 藍牙 (BLE) */
#define DEF_BT_CHANNEL 0
/* 本機 BLE 名稱前綴 (RC 不廣播, 僅設定裝置名) */
#define BT_NAME_PREFIX "KKBOX-RC-"
/* 掃描清單上限 */
#define BT_SCAN_MAX 24
/* 自訂 GATT UUID (需與接收機一致) */
#define BLE_SVC_UUID    "6b6f0001-4b4b-4000-8000-00805f9b34fb"
#define BLE_CMD_UUID    "6b6f0002-4b4b-4000-8000-00805f9b34fb"   /* RC -> RX (write) */
#define BLE_TEL_UUID    "6b6f0003-4b4b-4000-8000-00805f9b34fb"   /* RX -> RC (notify) */

#endif
