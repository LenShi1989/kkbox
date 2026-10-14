#ifndef RC_DISPLAY_H
#define RC_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "telem.h"

static Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
static bool gOledOk = false;

static inline void oledBegin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  gOledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (gOledOk) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println("KKBOX RC boot...");
    oled.display();
  }
}

static inline void oledShow() {
  if (!gOledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  /* 第 0 行: 標題 + 連線狀態 */
  oled.setCursor(0, 0);
  oled.print("KKBOX RC");
  oled.setCursor(80, 0);
  oled.print(gLinkUp ? "LINK" : "NO LINK");

  if (!gTelem.valid) {
    oled.setCursor(0, 24);
    oled.print("waiting telemetry");
    oled.display();
    return;
  }

  /* 第 1 行: 狀態 */
  oled.setCursor(0, 12);
  oled.print(gTelem.armed ? "ARMED" : "DISARM");
  oled.setCursor(72, 12);
  oled.print("THR ");
  oled.print(gTelem.thr);

  /* 陀螺三軸 */
  oled.setCursor(0, 26);
  oled.print("R ");
  oled.print(gTelem.roll, 1);
  oled.setCursor(0, 36);
  oled.print("P ");
  oled.print(gTelem.pitch, 1);
  oled.setCursor(0, 46);
  oled.print("Y ");
  oled.print(gTelem.yaw, 1);

  /* 馬達輸出 (右側) */
  oled.setCursor(72, 26);
  oled.print(gTelem.motor[0]);
  oled.setCursor(72, 36);
  oled.print(gTelem.motor[1]);
  oled.setCursor(72, 46);
  oled.print(gTelem.motor[2]);
  oled.setCursor(72, 56);
  oled.print(gTelem.motor[3]);

  oled.display();
}

#endif
