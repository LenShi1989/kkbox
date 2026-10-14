/*
 * WT32-ETH01 接收機 (Receiver) — KKBOX 飛控系統
 * ---------------------------------------------------------------
 * 版本 (Version): 2.0.0   (見 config.h 的 FW_VERSION)
 * 名稱 (Name):    WT32-ETH01 Receiver
 * ---------------------------------------------------------------
 * - 內建 LAN8720A RJ45 有線網路 + WiFi 無線網路 (可同時)
 * - SPIFFS 網頁介面 (ESPAsyncWebServer) + WebSocket 即時雙向
 * - 與 KKBOX 飛控板以 Serial2 (GPIO5/GPIO17, 115200) 交換 JSON
 * - 網頁: 系統狀態 / 網路設定 / PID 設定 / OTA / 使用者設定
 * - ESPAsyncWebServer + AsyncWebSocket + ElegantOTA
 * ---------------------------------------------------------------
 * 網頁需先登入 (預設 admin / kkbox)。
 *
 * 註: 提醒 (WT32-ETH01) GPIO16 為 PHY 電源, 不可他用。
 */

#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"   /* BOD (電壓不足偵測) 停用 */
#include "config.h"
#include "settings.h"
#include "kkbox.h"
#include "web.h"

static uint32_t bootMs = 0;
static bool     apStarted = false;

void setup() {
  /* 停用電壓不足偵測 (BOD): 避免 WiFi/RJ45(PHY) 瞬間電流造成 brownout reset */
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=================================");
  Serial.println(" " FW_NAME "  v" FW_VERSION);
  Serial.println("==========================================================");

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] 掛載失敗 (已嘗試格式化)");
  } else {
    Serial.printf("[SPIFFS] total=%u used=%u\n",
                  (unsigned)SPIFFS.totalBytes(), (unsigned)SPIFFS.usedBytes());
  }

  settingsLoad();

  /* ---------- RJ45 (LAN8720A) ---------- */
  bool ok = ETH.begin(ETH_PHY_LAN8720, ETH_ADDR, ETH_MDC, ETH_MDIO, ETH_POWER, ETH_CLOCK_GPIO0_IN);
  Serial.printf("[ETH] begin=%d dhcp=%d\n", ok, gSet.dhcp);
  if (!gSet.dhcp) {
    IPAddress ip, gw, mask, dns;
    ip.fromString(gSet.ip);
    gw.fromString(gSet.gw);
    mask.fromString(gSet.mask);
    dns.fromString(gSet.gw);
    ETH.config(ip, gw, mask, dns);
    Serial.printf("[ETH] static ip=%s\n", gSet.ip.c_str());
  }

  /* ---------- WiFi ---------- */
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("kkbox-receiver");
  if (gSet.ssid.length()) {
    WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    Serial.printf("[WiFi] connecting to %s ...\n", gSet.ssid.c_str());
  }

  /* ---------- 飛控板 UART & 網頁 ---------- */
  kkBegin();
  webSetup();

  bootMs = millis();
  Serial.println("[WEB] 伺服器已啟動 (port 80)");
}

void loop() {
  /* 讀取飛控板 JSON 行並廣播 */
  String line;
  while (kkReadLine(line)) {
    gKkLines++;
    if (line.indexOf("\"t\":") >= 0) {
      gLastTelemetry   = line;
      gLastTelemetryMs = millis();
      ws.textAll(line);
    } else if (line.indexOf("\"ack\":") >= 0) {
      gLastAck = line;
      ws.textAll(line);
    } else if (line.indexOf("\"evt\":") >= 0) {
      gLastEvt = line;
      ws.textAll(line);
    }
  }

  /* WiFi 斷線自動重連 */
  static uint32_t lastWifiTry = 0;
  if (gSet.ssid.length() && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiTry > 10000) {
      lastWifiTry = millis();
      WiFi.disconnect();
      WiFi.begin(gSet.ssid.c_str(), gSet.wpass.c_str());
    }
  }

  /* 無任何網路時啟動 SoftAP 供設定 */
  if (!apStarted && gSet.ssid.length() == 0 && !ETH.linkUp() &&
      millis() - bootMs > AP_FALLBACK_MS) {
    apStarted = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("[AP] %s -> %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }

  ws.cleanupClients();
  ElegantOTA.loop();

  if (gRebootAt && millis() > gRebootAt) {
    Serial.println("[SYS] 重新啟動以套用設定...");
    delay(50);
    ESP.restart();
  }

  delay(1);
}
