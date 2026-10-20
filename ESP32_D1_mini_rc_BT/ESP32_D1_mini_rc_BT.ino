/*
 * ESP32 D1 mini 藍牙遙控器 (RC-BT)
 * ---------------------------------------------------------------
 * 版本 (Version): 1.0.0   (見 config.h 的 FW_VERSION)
 * 名稱 (Name):    ESP32 D1 mini RC-BT
 * ---------------------------------------------------------------
 * - 傳統藍牙 (Classic BT / SPP) 用戶端, 開機自動連線 WT32 藍牙 Server
 * - 每 100ms 傳送 16 bytes 遙控資料, 接收 16 bytes 遙測
 * - SSD1306 128x64 OLED: TX / RX 頁面交替
 * - 純硬體遙控: 無 WiFi / 網頁 / OTA
 * ---------------------------------------------------------------
 * 開機流程 (沿用測試碼):
 *   1) 轉 VRC1 選無人機 ID -> 按下 SW1 確認
 *   2) 按住 SW1 期間持續校正搖桿 (VX/VY 軸), 放開 SW1 結束
 *   3) 按住 SW2 開機 = 跳過上述流程 (沿用已存的 ID/校正值)
 * 序列埠 (115200) 指令: help / status / sc / sn / sa / sv / id / fly / cal
 */

#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"   /* BOD (電壓不足偵測) 停用 */
#include "config.h"
#include "settings.h"
#include "proto.h"
#include "bt.h"
#include "display.h"

static char     sBuf[48];
static uint8_t  sLen = 0;
static bool     sSkipBoot = false;

/* ---------- LED: 連線時 300ms/700ms, 未連線時 100ms/900ms ---------- */
static bool     ledState = false;
static uint32_t ledLast = 0;

/* ---------- OLED 頁面交替 ---------- */
static uint32_t oledLast = 0;
static bool     oledPage = false;   /* false = TX, true = RX */

/* =============================================================
 * 開機流程: 無人機 ID 選擇 + 搖桿校正
 * ============================================================= */
static void bootFlow() {
  /* --- 1) 無人機 ID: 轉 VRC1 選擇, SW1 = ON 確認 --- */
  for (;;) {
    uint8_t id = (uint8_t)map(analogRead(PIN_VRC1), 0, 4095, 0, 255);
    gSet.flyId = id;
    oledBootFlyId(id);
    if (digitalRead(PIN_SW1) == LOW) break;   /* SW1 = ON */
    delay(100);
  }
  delay(100);

  /* --- 2) 搖桿校正: SW1 = ON 期間持續記錄 min/max --- */
  int tmax = 0, tmin = 4095, ymax = 0, ymin = 4095;
  int pmax = 0, pmin = 4095, rmax = 0, rmin = 4095;
  while (digitalRead(PIN_SW1) == LOW) {
    int v;
    v = analogRead(PIN_THROTTLE); if (v > tmax) tmax = v; if (v < tmin) tmin = v;
    v = analogRead(PIN_YAW);      if (v > ymax) ymax = v; if (v < ymin) ymin = v;
    v = analogRead(PIN_PITCH);    if (v > pmax) pmax = v; if (v < pmin) pmin = v;
    v = analogRead(PIN_ROLL);     if (v > rmax) rmax = v; if (v < rmin) rmin = v;
    oledBootCal();
    delay(20);
  }

  /* 覆蓋範圍過小 (輕點 SW1 就放開) 則沿用舊值, 避免校正壞掉 */
  if (tmax - tmin > 100) { gSet.tmin = tmin; gSet.tmax = tmax; }
  if (ymax - ymin > 100) { gSet.ymin = ymin; gSet.ymax = ymax; }
  if (pmax - pmin > 100) { gSet.pmin = pmin; gSet.pmax = pmax; }
  if (rmax - rmin > 100) { gSet.rmin = rmin; gSet.rmax = rmax; }
  settingsSaveCal();
  settingsSaveIds();

  Serial.println("[BOOT] 校正結果 (min/max):");
  Serial.printf("  THR %d/%d  YAW %d/%d\n", gSet.tmin, gSet.tmax, gSet.ymin, gSet.ymax);
  Serial.printf("  PIT %d/%d  ROL %d/%d\n", gSet.pmin, gSet.pmax, gSet.rmin, gSet.rmax);
  Serial.printf("[BOOT] RC id=%u  Fly id=%u\n", gSet.rcId, gSet.flyId);
}

/* =============================================================
 * 序列埠指令 (115200, 供設定/掃描/校正)
 * ============================================================= */
static void consoleHelp() {
  Serial.println(F("指令:"));
  Serial.println(F("  status        顯示連線/設定"));
  Serial.println(F("  sc            掃描可見藍牙裝置 (約5秒)"));
  Serial.println(F("  sn <name>     設定 Server 名稱 (例: sn KKBOX-RX-BT)"));
  Serial.println(F("  sa <mac>      設定 Server MAC (例: sa AA:BB:CC:DD:EE:FF, sa clear=清除)"));
  Serial.println(F("  sv            儲存 Server 設定並立即重連"));
  Serial.println(F("  id <0~255>    設定遙控器編號 (tx[12])"));
  Serial.println(F("  fly <0~255>   設定無人機編號 (tx[13])"));
  Serial.println(F("  cal           重新校正搖桿 (量測四軸 min/max)"));
  Serial.println(F("  help          顯示本說明"));
}

static void consoleStatus() {
  Serial.printf("[STATUS] BT=%s  %s\n",
                SerialBT.hasClient() ? "LINK" : "WAIT",
                gSet.serverMac.length() ? gSet.serverMac.c_str() : gSet.serverName.c_str());
  Serial.printf("  local=%s  rcId=%u  flyId=%u  tx#%lu  rx#%lu\n",
                btLocalName().c_str(), gSet.rcId, gSet.flyId,
                (unsigned long)gTxCount, (unsigned long)gRxCount);
  Serial.printf("  cal THR %d/%d  YAW %d/%d  PIT %d/%d  ROL %d/%d\n",
                gSet.tmin, gSet.tmax, gSet.ymin, gSet.ymax,
                gSet.pmin, gSet.pmax, gSet.rmin, gSet.rmax);
}

static void consoleCal() {
  int tmax = analogRead(PIN_THROTTLE), tmin = tmax;
  int ymax = analogRead(PIN_YAW),      ymin = ymax;
  int pmax = analogRead(PIN_PITCH),    pmin = pmax;
  int rmax = analogRead(PIN_ROLL),     rmin = rmax;
  Serial.println(F("[CAL] 量測中, 3 秒內請轉動四軸到底..."));
  uint32_t t0 = millis();
  while (millis() - t0 < 3000) {
    int v;
    v = analogRead(PIN_THROTTLE); if (v > tmax) tmax = v; if (v < tmin) tmin = v;
    v = analogRead(PIN_YAW);      if (v > ymax) ymax = v; if (v < ymin) ymin = v;
    v = analogRead(PIN_PITCH);    if (v > pmax) pmax = v; if (v < pmin) pmin = v;
    v = analogRead(PIN_ROLL);     if (v > rmax) rmax = v; if (v < rmin) rmin = v;
    delay(10);
  }
  if (tmax - tmin > 100) { gSet.tmin = tmin; gSet.tmax = tmax; }
  if (ymax - ymin > 100) { gSet.ymin = ymin; gSet.ymax = ymax; }
  if (pmax - pmin > 100) { gSet.pmin = pmin; gSet.pmax = pmax; }
  if (rmax - rmin > 100) { gSet.rmin = rmin; gSet.rmax = rmax; }
  settingsSaveCal();
  consoleStatus();
}

static void btScan() {
  Serial.println(F("[BT] 掃描中 (約 5 秒)..."));
  SerialBT.discoverAsync([](BTAdvertisedDevice* d) {
    Serial.printf("  %s  %s\n", d->getAddress().toString().c_str(), d->getName().c_str());
  });
  delay(5000);
  SerialBT.discoverAsyncStop();
  Serial.println(F("[BT] 掃描結束 (名稱請用 sn 指令設定)"));
}

static void consoleRun(char* line) {
  char* cmd = strtok(line, " ");
  if (!cmd) return;
  if (!strcmp(cmd, "help")) { consoleHelp(); return; }
  if (!strcmp(cmd, "status")) { consoleStatus(); return; }
  if (!strcmp(cmd, "sc")) { btScan(); return; }
  if (!strcmp(cmd, "sv")) { settingsSaveBt(); btDisconnect(); gBtLastRetry = millis(); return; }
  if (!strcmp(cmd, "cal")) { consoleCal(); return; }
  if (!strcmp(cmd, "sn")) {
    char* p = strtok(NULL, "");
    if (!p || !*p) { Serial.println(F("用法: sn <name>")); return; }
    gSet.serverName = p;
    gSet.serverMac = "";
    Serial.printf("[BT] Server 名稱 = %s (sv 儲存並重連)\n", p);
    return;
  }
  if (!strcmp(cmd, "sa")) {
    char* p = strtok(NULL, "");
    if (!p || !*p) { Serial.println(F("用法: sa <mac> 或 sa clear")); return; }
    if (!strcmp(p, "clear")) gSet.serverMac = "";
    else gSet.serverMac = p;
    Serial.printf("[BT] Server MAC = %s (sv 儲存並重連)\n", gSet.serverMac.c_str());
    return;
  }
  if (!strcmp(cmd, "id")) {
    char* p = strtok(NULL, "");
    if (!p || !*p) { Serial.println(F("用法: id <0~255>")); return; }
    gSet.rcId = (uint8_t)constrain(atoi(p), 0, 255);
    settingsSaveIds();
    Serial.printf("[BT] 遙控器編號 = %u\n", gSet.rcId);
    return;
  }
  if (!strcmp(cmd, "fly")) {
    char* p = strtok(NULL, "");
    if (!p || !*p) { Serial.println(F("用法: fly <0~255>")); return; }
    gSet.flyId = (uint8_t)constrain(atoi(p), 0, 255);
    settingsSaveIds();
    Serial.printf("[BT] 無人機編號 = %u\n", gSet.flyId);
    return;
  }
  Serial.printf("未知指令: %s (help 查看指令)\n", cmd);
}

static void consoleLoop() {
  while (Serial.available()) {
    int c = Serial.read();
    if (c < 0) break;
    if (c == '\r' || c == '\n') {
      if (sLen) { sBuf[sLen] = 0; consoleRun(sBuf); sLen = 0; }
    } else if (sLen < sizeof sBuf - 1) {
      sBuf[sLen++] = (char)c;
    }
  }
}

/* =============================================================
 * setup / loop
 * ============================================================= */
void setup() {
  /* 停用電壓不足偵測 (BOD): 避免 OLED 瞬間電流造成 brownout reset */
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("==========================================================");
  Serial.printf("  %s %s\n", FW_NAME, FW_VERSION);
  Serial.println("==========================================================");

  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);
  pinMode(PIN_SW4, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  analogReadResolution(12);

  settingsLoad();
  oledBegin();

  /* 按住 SW2 開機 = 跳過 ID/校正流程 (SW1 損壞或不需重校時用) */
  sSkipBoot = (digitalRead(PIN_SW2) == LOW);
  if (sSkipBoot) {
    Serial.println("[BOOT] SW2 = ON, 跳過 ID / 校正流程");
  } else {
    bootFlow();
  }

  btBegin();
  gBtLastRetry = millis();
  consoleHelp();
}

void loop() {
  /* LED: 未連線閃爍較快 (100/900), 連線時 300/700 */
  uint32_t onMs  = gBtLinked ? 300 : 100;
  uint32_t offMs = gBtLinked ? 700 : 900;
  if (millis() - ledLast >= (ledState ? onMs : offMs)) {
    ledLast = millis();
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState ? HIGH : LOW);
  }

  btLoop();
  consoleLoop();

  /* OLED: 每 OLED_MS 交替 TX / RX 頁 */
  if (millis() - oledLast >= OLED_MS) {
    oledLast = millis();
    oledPage = !oledPage;
    if (oledPage) oledShowRx();
    else          oledShowTx();
  }

  delay(1);
}
