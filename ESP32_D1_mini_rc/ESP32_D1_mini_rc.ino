/*
 * ESP32 D1 mini 遙控器 (RC) — KKBOX 飛控系統
 * ---------------------------------------------------------------
 * 版本 (Version): 2.0.0   (見 config.h 的 FW_VERSION)
 * 名稱 (Name):    ESP32 D1 mini RC
 * ---------------------------------------------------------------
 * - WiFi 連線
 * - WebSocket 用戶端連線 WT32-ETH01 接收機, 收發遙測 / 命令
 * - SSD1306 128x64 OLED 顯示遙測
 * - SPIFFS 網頁介面: 系統狀態 / 網路設定 / 模擬搖桿 / OTA / 使用者設定
 * ---------------------------------------------------------------
 * 網頁預設帳密 admin / kkbox。首次使用請於「網路設定」填入接收機 IP 與 Token。
 */

#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"   /* BOD (電壓不足偵測) 停用 */
#include "config.h"
#include "settings.h"
#include <ESPAsyncWebServer.h>

/* 全域伺服器物件 (需在 link.h / web.h 之前定義) */
static AsyncWebServer server(WEB_PORT);
static AsyncWebSocket localWs("/ws");

#include "telem.h"
#include "display.h"
#include "link.h"
#include "web.h"

static uint32_t lastOled = 0;

void setup() {
  /* 停用電壓不足偵測 (BOD): 避免 WiFi/OLED 瞬間電流造成 brownout reset */
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("==========================================================");
  Serial.printf("  %s %s\n", FW_NAME, FW_VERSION);
  Serial.println("==========================================================");

  if (!SPIFFS.begin(true)) Serial.println("[SPIFFS] 掛載失敗");
  settingsLoad();
  oledBegin();

  /* ---------- WiFi: STA + AP 共存 ---------- */
  /* 一律啟動 SoftAP, 確保任何情況下都能進入設定頁; STA 則連線外部 WiFi */
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("kkbox-rc");
  {
    /* SoftAP 固定 192.168.5.x, 與接收機 192.168.4.x 區隔 */
    IPAddress apIp, apMask;
    apIp.fromString(AP_IP_ADDR);
    apMask.fromString("255.255.255.0");
    WiFi.softAPConfig(apIp, apIp, apMask);
    String ap = apSsid();
    WiFi.softAP(ap.c_str(), AP_PASS);
    Serial.printf("[AP] %s -> %s\n", ap.c_str(), WiFi.softAPIP().toString().c_str());
  }
  if (gSet.ssid.length()) {
    WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    Serial.printf("[WiFi] connecting to %s ...\n", gSet.ssid.c_str());
  }

  webSetup();
  if (gSet.rxHost.length()) linkBegin();
  else Serial.println("[LINK] 尚未設定接收機, 請由網頁設定");

  if (gOledOk) { oled.clearDisplay(); oled.display(); }
  Serial.println("[WEB] 伺服器已啟動 (port 80)");
}

void loop() {
  /* WiFi 斷線自動重連 (SoftAP 持續運作, 僅重連 STA) */
  static uint32_t lastWifiTry = 0;
  if (gSet.ssid.length() && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiTry > 10000) {
      lastWifiTry = millis();
      WiFi.disconnect();
      WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    }
  }

  linkLoop();
  localWs.cleanupClients();
  ElegantOTA.loop();

  if (millis() - lastOled > 200) {
    lastOled = millis();
    oledShow();
  }

  if (gRebootAt && millis() > gRebootAt) {
    Serial.println("[SYS] 重新啟動...");
    delay(50);
    ESP.restart();
  }

  delay(1);
}
