#ifndef RC_BT_CONFIG_H
#define RC_BT_CONFIG_H

/* =============================================================
 * ESP32 D1 mini 藍牙遙控器 (RC-BT) 設定
 * -------------------------------------------------------------
 * 版本 (Version): 1.0.0   (見 FW_VERSION)
 * 名稱 (Name):    ESP32 D1 mini RC-BT
 * ---------------------------------------------------------------
 * 功能:
 *   - 傳統藍牙 (Classic BT / SPP) 用戶端: 開機自動連線 WT32 藍牙 Server
 *   - 每 100ms 傳送 16 bytes 遙控資料, 接收 16 bytes 遙測
 *   - SSD1306 128x64 OLED 顯示 (TX / RX 電視牆式交替)
 *   - 純硬體遙控: 無 WiFi / 網頁 / OTA
 * -------------------------------------------------------------
 * 參考: test/esp32_server---TESTOK-----20250710 (搖桿腳位與 16-byte 協定)
 * 注意: SW1 = GPIO10 在 ESP32-WROOM 模組屬 Flash 腳, 若開機異常請改
 *       config.h 的 PIN_SW1 並同步接線。
 * ============================================================= */

#include <Arduino.h>

#define FW_NAME "ESP32 D1 mini RC-BT"
#define FW_VERSION "1.0.0"

/* ---------- OLED (SSD1306 I2C, 沿用測試碼接線) ---------- */
#define OLED_SDA 17
#define OLED_SCL 16
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64

/* ---------- 搖桿 / 類比 (ADC 0~4095) ---------- */
#define PIN_THROTTLE 39   /* 油門 (VTX) */
#define PIN_YAW      35   /* 偏航 (VTY) */
#define PIN_PITCH    33   /* 俯仰 (VPIT) */
#define PIN_ROLL     34   /* 橫滾 (VROL) */
#define PIN_VRC1     13   /* 可變電阻 1 (選擇/設定用) */
#define PIN_VRC2     25
#define PIN_VRC3     32
#define PIN_VRC4     12

/* ---------- 開關 (INPUT_PULLUP, LOW = ON) ---------- */
#define PIN_SW1 10
#define PIN_SW2 18
#define PIN_SW3 19
#define PIN_SW4 23

#define PIN_LED 2         /* 板上 LED, HIGH = 亮 */

/* ---------- 傳統藍牙 (Classic BT SPP, 本機為 client) ---------- */
/* 本機廣播/裝置名稱前綴 = 前綴 + MAC 後 4 碼 */
#define BT_NAME_PREFIX "KKBOX-RCBT-"
/* 預設要自動連線的 WT32 藍牙 Server 名稱 (可由序列埠 sn 指令更改) */
#define DEF_BT_SERVER_NAME "KKBOX-RX-BT"
/* 空字串 = 不設定 (Just Works); 若 Server 有設 PIN, 改成字串如 "1234" */
#define BT_PIN ""
/* 掃描結果顯示上限 */
#define BT_SCAN_MAX 8

/* ---------- 傳送節奏 ---------- */
#define BT_SEND_MS 100      /* 每 100ms 傳送一次 16 bytes */
#define BT_RETRY_MS 3000    /* 斷線後重試連線間隔 */
#define OLED_MS 3000        /* OLED TX/RX 頁面交替間隔 */

/* ---------- 預設 ID ---------- */
#define DEF_RC_ID 3         /* 遙控器編號 (tx[12]) */
#define DEF_FLY_ID 3        /* 目標無人機編號 (tx[13]) */

#endif
