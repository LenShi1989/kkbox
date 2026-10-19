#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

/* =============================================================
 * WT32-ETH01 接收機 (Receiver) 設定
 * -------------------------------------------------------------
 * 版本 (Version): 2.0.0   (見 FW_VERSION)
 * 名稱 (Name):    WT32-ETH01 Receiver
 * -------------------------------------------------------------
 * 功能:
 *   - 透過 Serial2 (GPIO5=RX, GPIO17=TX) 與 KKBOX 飛控板以 JSON 通訊
 *   - 內建 LAN8720A RJ45 有線網路 + WiFi 無線網路
 *   - 藍牙 BLE (NimBLE) GATT 伺服器, 供 RC 以 BLE 傳收命令 / 遙測
 *   - SPIFFS 網頁介面 (ESPAsyncWebServer) + WebSocket 即時雙向
 *   - WiFi / RJ45 / PID / 使用者 / OTA 設定
 * ============================================================= */

#include <Arduino.h>

#define FW_NAME    "WT32-ETH01 Receiver"
#define FW_VERSION "2.0.0"

/* ---------- 與 KKBOX 飛控板的 UART ---------- */
#define KK_RX_PIN   5
#define KK_TX_PIN   17
/* 9600: 飛控板 RX 為 8MHz SoftwareSerial, 實測僅 9600 能可靠收到命令 */
#define KK_BAUD     9600

/* ---------- 內建 LAN8720A RJ45 (WT32-ETH01) ---------- */
#define ETH_ADDR    1
#define ETH_MDC     23
#define ETH_MDIO    18
#define ETH_POWER   16
/* 一般 WT32-ETH01 使用振盪器輸入到 GPIO0 */
/* ETH_CLOCK_GPIO0_IN 由 ESP32 core 的 ETH.h 提供 */

/* ---------- Web / AP ---------- */
#define WEB_PORT        80
/* SoftAP 一律啟動, 與 STA 共存; SSID = 前綴 + MAC 後 4 碼 (見 settings.h apSsid()) */
#define AP_SSID_PREFIX  "KKBOX-Receiver-"
#define AP_PASS         "12345678"    /* SoftAP 密碼, 至少 8 碼 */
#define SESSION_MAX     8

/* ---------- 預設帳密 ---------- */
#define DEF_USER  "admin"
#define DEF_PASS  "admin"

/* ---------- 藍牙 BLE (NimBLE) ---------- */
/* 廣播名稱 = 前綴 + MAC 後 4 碼 (見 settings.h btName()) */
#define BT_NAME_PREFIX  "KKBOX-RX-"
/* 自訂 GATT UUID (兩端需一致); ...626f = "kbo" */
#define BLE_SVC_UUID    "6b6f0001-4b4b-4000-8000-00805f9b34fb"
#define BLE_CMD_UUID    "6b6f0002-4b4b-4000-8000-00805f9b34fb"   /* RC -> RX (write) */
#define BLE_TEL_UUID    "6b6f0003-4b4b-4000-8000-00805f9b34fb"   /* RX -> RC (notify) */

/* ---------- 預設 PID (與 kkbox/config.h 一致) ---------- */
/* roll/pitch KI 即自動配平積分 (auto-trim): 搖桿置中時緩慢學習配平量 */
#define DEF_ROLL_KP   3.0f
#define DEF_ROLL_KI   0.08f
#define DEF_ROLL_KD   8.0f
#define DEF_PITCH_KP  3.0f
#define DEF_PITCH_KI  0.08f
#define DEF_PITCH_KD  8.0f
#define DEF_YAW_KP    4.0f
#define DEF_YAW_KI    0.5f
#define DEF_YAW_KD    0.0f

#endif
