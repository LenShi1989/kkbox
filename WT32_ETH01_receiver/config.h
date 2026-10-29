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
 *   - 藍牙 Server (二擇一, 網頁切換): BLE (NimBLE GATT) 或
 *     傳統藍牙 (Classic BT / SPP, 供 ESP32_D1_mini_rc_BT 以 16-byte 協定連線)
 *   - SPIFFS 網頁介面 (ESPAsyncWebServer) + WebSocket 即時雙向
 *   - WiFi / RJ45 / 馬達模式 / 使用者 / OTA 設定 (PID 設定已移至遙控器)
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

/* ---------- 藍牙通道 (三擇一) ---------- */
/* ESP32 只能用一個藍牙主機堆疊: NimBLE (BLE) 與 Classic BT (SPP) 無法同開,
 * 故以 NVS 設定 + 網頁切換。0 = BLE, 1 = 傳統藍牙 SPP, 2 = 關閉 (不啟動)。 */
#define BT_MODE_BLE 0
#define BT_MODE_SPP 1
#define BT_MODE_OFF 2
#define DEF_BT_MODE BT_MODE_BLE

/* 傳統藍牙 SPP 伺服器名稱 (ESP32_D1_mini_rc_BT 預設連線目標) */
#define SPP_NAME "KKBOX-RX-BT"
/* 空字串 = 不設定 (Just Works); 若 RC 端有設 PIN 改成字串如 "1234" */
#define SPP_PIN  ""
#define SPP_DRONE_ID 3     /* 遙測封包 [14]: 無人機編號 (0~255) */

/* 16-byte 遙控資料 -> kkbox 指令 (9600baud 串列有 ~960 字元/秒上限,
 * 以 SPP_CMD_MS / SPP_THR_MS 限速, 避免排擠遙測與指令) */
#define SPP_CMD_MS   100   /* setpoints 最小下傳間隔 (10Hz) */
#define SPP_THR_MS   200   /* throttle 最小下傳間隔 (5Hz) */
#define SPP_HB_MS    500   /* 心跳間隔 (kkbox 2 秒無命令即 failsafe 上鎖) */
#define SPP_TEL_MS   200   /* 遙測 16-byte 送給 RC-BT 的間隔 */
#define SPP_DEADBAND 2     /* 搖桿中心死區 (byte, 128±2 視為 0) */
#define SPP_MAX_SET  200   /* 角速率滿刻度 deg/s (與網頁搖桿 RANGE 一致) */

/* KKBOX motor[] 順序固定為 M1..M8：M1=FL(CCW), M2=FR(CW),
 * M3=RR(CCW), M4=RL(CW); M5..M8 為各位置 X8 下槳。 */

#endif
