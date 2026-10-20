#ifndef RC_BT_DISPLAY_H
#define RC_BT_DISPLAY_H

/* =============================================================
 * SSD1306 128x64 OLED 顯示
 *   - 開機流程: 無人機 ID 選擇 -> 搖桿校正 (SW1 按住)
 *   - 連線後: 每 OLED_MS 交替顯示 TX (遙控資料) / RX (遙測)
 * ============================================================= */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config.h"
#include "settings.h"
#include "proto.h"

static Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
static bool gOledOk = false;

static void oledBegin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  gOledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!gOledOk) { Serial.println("[OLED] 啟動失敗"); return; }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.display();
}

/* ---------- 開機: 選擇目標無人機 ID (SW1 按下確認) ---------- */
static void oledBootFlyId(uint8_t id) {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.printf("FlyID:%3d", id);
  oled.setCursor(0, 32);
  oled.setTextSize(1);
  oled.print(F("SW1 = ON to confirm"));
  oled.display();
}

/* ---------- 開機: 搖桿校正畫面 ---------- */
static void oledBootCal() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(F("Calibrating sticks"));
  oled.println(F("Hold SW1 to keep"));
  oled.println(F("Release SW1 to end"));
  oled.println();
  oled.printf("THR %4d  YAW %4d\n", analogRead(PIN_THROTTLE), analogRead(PIN_YAW));
  oled.printf("PIT %4d  ROL %4d\n", analogRead(PIN_PITCH),    analogRead(PIN_ROLL));
  oled.display();
}

/* ---------- TX 頁: 本機送出的遙控資料 ---------- */
static void oledShowTx() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("THR:%3d  YAW:%3d\n", gTx[0], gTx[1]);
  oled.printf("PIT:%3d  ROL:%3d\n", gTx[2], gTx[3]);
  oled.printf("V1:%3d V2:%3d V3:%3d V4:%3d\n", gTx[4], gTx[9], gTx[10], gTx[11]);
  oled.printf("SW1:%s SW2:%s\n", gTx[5] ? "OFF" : " ON", gTx[6] ? "OFF" : " ON");
  oled.printf("SW3:%s SW4:%s\n", gTx[7] ? "OFF" : " ON", gTx[8] ? "OFF" : " ON");
  oled.printf("RC:%3d  Fly:%3d\n", gTx[12], gTx[13]);
  oled.printf("%s  TX#%lu", gBtLinked ? "BT:LINK" : "BT:WAIT", (unsigned long)gTxCount);
  oled.display();
}

/* ---------- RX 頁: 對方回傳的遙測 ---------- */
static void oledShowRx() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("Mot1:%3d Mot2:%3d\n", gRx[0], gRx[1]);
  oled.printf("Mot3:%3d Mot4:%3d\n", gRx[2], gRx[3]);
  oled.printf("Mot5:%3d Mot6:%3d\n", gRx[4], gRx[5]);
  oled.printf("Mot7:%3d Mot8:%3d\n", gRx[6], gRx[7]);
  oled.printf("PIT:%3d ROL:%3d\n", gRx[8], gRx[9]);
  oled.printf("YAW:%3d TMP:%3d\n", gRx[10], gRx[11]);
  int16_t alt = (int16_t)(((uint16_t)gRx[12] << 8) | gRx[13]);
  oled.printf("ALT:%d  Id:%3d\n", alt, gRx[14]);
  oled.printf("%s  RX#%lu", gBtLinked ? "BT:LINK" : "BT:WAIT", (unsigned long)gRxCount);
  oled.display();
}

#endif
