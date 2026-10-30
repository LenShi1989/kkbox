#ifndef RC_DISPLAY_H
#define RC_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "settings.h"
#include "telem.h"
#include "input.h"

/* =============================================================
 * SSD1306 128x64 OLED
 *   - 開機: 無人機 ID 選擇 / 搖桿校正
 *   - 開機後: VRC1 四段切換頁面
 *       0 = 飛行遙測   1 = 硬體輸入
 *       2 = 網路資訊   3 = ID / 校正值
 * ============================================================= */

static Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
static bool gOledOk = false;

static inline void oledBegin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  gOledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!gOledOk) { Serial.println("[OLED] 啟動失敗"); return; }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("KKBOX RC boot...");
  oled.display();
}

static inline void oledText(int x, int y, const String& s, uint8_t size = 1) {
  if (!gOledOk) return;
  oled.setTextSize(size);
  oled.setCursor(x, y);
  oled.print(s);
}

/* ---------- 開機: 選擇目標無人機 ID (SW1 按下確認) ---------- */
static inline void oledBootFlyId(uint8_t id, int raw) {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.printf("FlyID:%3d", id);
  oled.setTextSize(1);
  oled.setCursor(0, 26);
  oled.print("Turn VRC1 to select");
  oled.setCursor(0, 38);
  oled.print("SW1 = ON to confirm");
  oled.setCursor(0, 52);
  oled.printf("VRC1 raw: %4d", raw);
  oled.display();
}

/* ---------- 開機: 搖桿校正 (按住 SW1, 放開結束) ---------- */
static inline void oledBootCal() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Calibrating sticks");
  oled.println("Hold SW1, move all");
  oled.println("sticks to both ends");
  oled.println("Release SW1 = end");
  oled.printf("T%4d-%4d Y%4d-%4d\n", gCalTmin, gCalTmax, gCalYmin, gCalYmax);
  oled.printf("P%4d-%4d R%4d-%4d\n", gCalPmin, gCalPmax, gCalRmin, gCalRmax);
  oled.printf("now %4d %4d %4d %4d", gHw.rawT, gHw.rawY, gHw.rawP, gHw.rawR);
  oled.display();
}

static inline void oledBootSkip() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("SW2 = ON");
  oled.println("Skip boot flow");
  oled.println("Use saved FlyID/cal");
  oled.println();
  oled.printf("FlyID: %3d\n", gSet.flyId);
  oled.printf("Cal   : %s\n", gSet.calValid ? "OK" : "NOT CAL");
  oled.display();
}

/* ---------- 共用標頭 ---------- */
static inline void oledHeader(const char* title) {
  if (!gOledOk) return;
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(title);
  oled.setCursor(84, 0);
  oled.printf("p%u/%u", (unsigned)(gPage + 1), (unsigned)OLED_PAGES);
  oled.drawFastHLine(0, 9, OLED_W, SSD1306_WHITE);
}

/* ---------- 頁面 0: 飛行遙測 ---------- */
static inline void oledPageFlight() {
  oled.clearDisplay();
  oledHeader("FLIGHT");
  oled.setCursor(0, 11);
  oled.printf("ID:%3u  %s", (unsigned)gSet.flyId,
               gLinkUp ? "LINK" : "----");
  oled.setCursor(0, 21);
  oled.printf("%s  THR:%4ld", gTelem.armed ? "ARMED " : "DISARM", gTelem.thr);
  oled.setCursor(0, 31);
  oled.printf("R:%+5.0f P:%+5.0f", gTelem.roll, gTelem.pitch);
  oled.setCursor(0, 41);
  oled.printf("Y:%+5.0f  %s", gTelem.yaw, inputSourceName(gCtrlActive));
  oled.setCursor(0, 51);
  oled.printf("M1:%4d M2:%4d", gTelem.motor[0], gTelem.motor[1]);
  oled.setCursor(0, 61);
  oled.printf("M3:%4d M4:%4d", gTelem.motor[2], gTelem.motor[3]);
  oled.display();
}

/* ---------- 頁面 1: 硬體輸入 ---------- */
static inline void oledPageInput() {
  oled.clearDisplay();
  oledHeader("HARDWARE");
  oled.setCursor(0, 11);
  oled.printf("THR %4d -> %4ld", gHw.rawT, gHw.thr);
  oled.setCursor(0, 21);
  oled.printf("YAW %4d -> %+4.0f", gHw.rawY, gHw.yaw);
  oled.setCursor(0, 31);
  oled.printf("PIT %4d -> %+4.0f", gHw.rawP, gHw.pitch);
  oled.setCursor(0, 41);
  oled.printf("ROL %4d -> %+4.0f", gHw.rawR, gHw.roll);
  oled.setCursor(0, 51);
  oled.printf("SW %d%d%d%d  VRC1:%3d", gHw.sw1, gHw.sw2, gHw.sw3, gHw.sw4,
               (unsigned)gHw.vrc1Id);
  oled.setCursor(0, 61);
  oled.printf("SRC:%s CAL:%s", inputSourceName(gCtrlActive),
               gSet.calValid ? "OK" : "NO");
  oled.display();
}

/* ---------- 頁面 2: 網路資訊 ---------- */
static inline void oledPageNet() {
  bool wc = (WiFi.status() == WL_CONNECTED);
  oled.clearDisplay();
  oledHeader("NETWORK");
  oled.setCursor(0, 11);
  oled.print("STA:");
  oled.print((wc ? WiFi.SSID() : gSet.ssid).substring(0, 17));
  oled.setCursor(0, 21);
  oled.printf("IP :%s", wc ? WiFi.localIP().toString().c_str() : "-");
  oled.setCursor(0, 31);
  oled.printf("RSSI:%d dBm", wc ? WiFi.RSSI() : 0);
  oled.setCursor(0, 41);
  oled.print("AP :");
  oled.print(apSsid().substring(0, 17));
  oled.setCursor(0, 51);
  oled.printf("AP :%s", WiFi.softAPIP().toString().c_str());
  oled.setCursor(0, 61);
  oled.print("RX :");
  oled.print(gSet.rxHost.substring(0, 13));
  oled.printf(":%u", gSet.rxPort);
  oled.display();
}

/* ---------- 頁面 3: ID / 校正值 ---------- */
static inline void oledPageCal() {
  oled.clearDisplay();
  oledHeader("ID / CAL");
  oled.setCursor(0, 11);
  oled.printf("FlyID:%3u  RCID:%3u", (unsigned)gSet.flyId, (unsigned)gSet.rcId);
  oled.setCursor(0, 21);
  oled.printf("THR %4d - %4d", gSet.tmin, gSet.tmax);
  oled.setCursor(0, 31);
  oled.printf("YAW %4d - %4d", gSet.ymin, gSet.ymax);
  oled.setCursor(0, 41);
  oled.printf("PIT %4d - %4d", gSet.pmin, gSet.pmax);
  oled.setCursor(0, 51);
  oled.printf("ROL %4d - %4d", gSet.rmin, gSet.rmax);
  oled.setCursor(0, 61);
  oled.printf("CAL:%s  VRC1:%3u", gSet.calValid ? "OK" : "NO",
               (unsigned)gHw.vrc1Id);
  oled.display();
}

/* ---------- 主更新 (依 VRC1 頁面) ---------- */
static inline void oledShow() {
  if (!gOledOk) return;
  switch (gPage) {
    case 1:  oledPageInput();  break;
    case 2:  oledPageNet();    break;
    case 3:  oledPageCal();    break;
    default: oledPageFlight(); break;
  }
}

#endif