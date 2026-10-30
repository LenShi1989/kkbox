#ifndef RC_INPUT_H
#define RC_INPUT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>
#include "config.h"
#include "settings.h"

/* =============================================================
 * 硬體遙控輸入: 四軸搖桿 (ADC1) + SW1~SW4 + VRC1
 * 軟體遙控: 網頁模擬搖桿 (由 web.h 呼叫 inputHandleCmd)
 * ============================================================= */

struct HwState {
  int     rawT, rawY, rawP, rawR, rawVrc1;
  float   roll, pitch, yaw;   /* deg/s */
  long    thr;                /* us */
  bool    sw1, sw2, sw3, sw4; /* true = ON (LOW) */
  uint8_t vrc1Id;             /* 0~255 */
  uint8_t vrc1Zone;           /* 0~3 */
};

static HwState  gHw;
static uint8_t  gPage = 0;          /* 目前 OLED 頁面 (VRC1 控制) */
static uint8_t  gCtrlActive = CTRL_HARDWARE;

/* 軟體搖桿狀態 */
static uint32_t gSoftMs = 0;
static float    gSoftRoll = 0, gSoftPitch = 0, gSoftYaw = 0;
static long     gSoftThr = THR_MIN_US;

/* 校正狀態 */
static bool gCalActive = false;
static int  gCalTmin, gCalTmax, gCalYmin, gCalYmax;
static int  gCalPmin, gCalPmax, gCalRmin, gCalRmax;

static inline int adcRead(uint8_t pin) {
  int32_t sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) sum += analogRead(pin);
  return (int)(sum / ADC_SAMPLES);
}

static inline void inputBegin() {
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);
  pinMode(PIN_SW4, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  analogReadResolution(12);
  /* 11dB 衰減: 量測範圍約 0~3.1V, 適合 3.3V 的可變電阻 */
  analogSetPinAttenuation(PIN_THROTTLE, ADC_11db);
  analogSetPinAttenuation(PIN_YAW,      ADC_11db);
  analogSetPinAttenuation(PIN_PITCH,    ADC_11db);
  analogSetPinAttenuation(PIN_ROLL,     ADC_11db);
  analogSetPinAttenuation(PIN_VRC1,     ADC_11db);
}

/* ---------- 校正 ---------- */
static inline void inputCalStart() {
  gCalActive = true;
  gCalTmin = gCalTmax = adcRead(PIN_THROTTLE);
  gCalYmin = gCalYmax = adcRead(PIN_YAW);
  gCalPmin = gCalPmax = adcRead(PIN_PITCH);
  gCalRmin = gCalRmax = adcRead(PIN_ROLL);
}

static inline void inputCalSample() {
  if (!gCalActive) return;
  int v;
  v = adcRead(PIN_THROTTLE); if (v < gCalTmin) gCalTmin = v; if (v > gCalTmax) gCalTmax = v;
  v = adcRead(PIN_YAW);      if (v < gCalYmin) gCalYmin = v; if (v > gCalYmax) gCalYmax = v;
  v = adcRead(PIN_PITCH);    if (v < gCalPmin) gCalPmin = v; if (v > gCalPmax) gCalPmax = v;
  v = adcRead(PIN_ROLL);     if (v < gCalRmin) gCalRmin = v; if (v > gCalRmax) gCalRmax = v;
}

/* 結束校正: 只採計行程足夠 (>200 counts) 的軸, 四軸都足夠才標記 calValid */
static inline bool inputCalFinish() {
  if (!gCalActive) return gSet.calValid;
  inputCalSample();
  gCalActive = false;

  bool okT = (gCalTmax - gCalTmin) > 200;
  bool okY = (gCalYmax - gCalYmin) > 200;
  bool okP = (gCalPmax - gCalPmin) > 200;
  bool okR = (gCalRmax - gCalRmin) > 200;

  if (okT) { gSet.tmin = gCalTmin; gSet.tmax = gCalTmax; }
  if (okY) { gSet.ymin = gCalYmin; gSet.ymax = gCalYmax; }
  if (okP) { gSet.pmin = gCalPmin; gSet.pmax = gCalPmax; }
  if (okR) { gSet.rmin = gCalRmin; gSet.rmax = gCalRmax; }
  if (okT && okY && okP && okR) gSet.calValid = true;

  settingsSaveCal();
  Serial.printf("[CAL] THR %d/%d YAW %d/%d PIT %d/%d ROL %d/%d -> %s\n",
                gSet.tmin, gSet.tmax, gSet.ymin, gSet.ymax,
                gSet.pmin, gSet.pmax, gSet.rmin, gSet.rmax,
                gSet.calValid ? "OK" : "行程不足, 保留舊值");
  return gSet.calValid;
}

/* ---------- 對應 ---------- */
static inline long mapThrottle(int raw) {
  int lo = gSet.tmin, hi = gSet.tmax;
  if (hi - lo < 200) { lo = DEF_ADC_MIN; hi = DEF_ADC_MAX; }
  long v = map((long)raw, lo, hi, (long)THR_MIN_US, (long)THR_MAX_US);
  return constrain(v, (long)THR_MIN_US, (long)THR_MAX_US);
}

/* 以校正範圍中點為 0, 線性對應到 -STICK_RANGE ~ +STICK_RANGE, 含置中死區 */
static inline float mapStick(int raw, int lo, int hi) {
  if (hi - lo < 200) { lo = DEF_ADC_MIN; hi = DEF_ADC_MAX; }
  float center = (lo + hi) * 0.5f;
  float half;
  float n;
  if (raw >= center) {
    half = hi - center;
    n = half > 1 ? (raw - center) / half : 0;
  } else {
    half = center - lo;
    n = half > 1 ? (raw - center) / half : 0;
  }
  if (n > 1) n = 1;
  if (n < -1) n = -1;
  float a = fabsf(n);
  if (a < STICK_DEADBAND) {
    n = 0;
  } else {
    n = (n > 0 ? 1.0f : -1.0f) * (a - STICK_DEADBAND) / (1.0f - STICK_DEADBAND);
  }
  return n * STICK_RANGE;
}

/* ---------- VRC1: 4 段頁面 (含遲滯) ---------- */
static inline uint8_t vrc1Zone(int raw) {
  uint8_t z = 0;
  if (raw >= 3072)      z = 3;
  else if (raw >= 2048) z = 2;
  else if (raw >= 1024) z = 1;

  if (z > gPage && raw < (int)z * 1024 + VRC1_HYST) z = gPage;
  if (z < gPage && raw > (int)gPage * 1024 - VRC1_HYST) z = gPage;
  return z;
}

/* ---------- 軟體搖桿命令 ----------
 * 回傳 false = 目前為手動「硬體」模式且已校正, 拒絕軟體搖桿命令 */
static inline bool inputHandleCmd(const String& msg) {
  JsonDocument doc;
  if (deserializeJson(doc, msg)) return true;
  const char* cmd = doc["cmd"] | "";
  if (!strcmp(cmd, "setpoints")) {
    if (gSet.calValid && gSet.ctrlSource == CTRL_HARDWARE) return false;
    gSoftRoll  = doc["roll"]  | gSoftRoll;
    gSoftPitch = doc["pitch"] | gSoftPitch;
    gSoftYaw   = doc["yaw"]   | gSoftYaw;
    gSoftMs = millis();
  } else if (!strcmp(cmd, "throttle")) {
    if (gSet.calValid && gSet.ctrlSource == CTRL_HARDWARE) return false;
    gSoftThr = doc["value"] | gSoftThr;
    gSoftMs = millis();
  }
  return true;
}

static inline const char* inputSourceName(uint8_t s) {
  return s == CTRL_HARDWARE ? "硬體" : (s == CTRL_SOFTWARE ? "軟體" : "自動");
}

/* ---------- 主讀取: 更新 gHw 與目前有效控制值 ---------- */
static inline void inputRead() {
  gHw.rawT    = adcRead(PIN_THROTTLE);
  gHw.rawY    = adcRead(PIN_YAW);
  gHw.rawP    = adcRead(PIN_PITCH);
  gHw.rawR    = adcRead(PIN_ROLL);
  gHw.rawVrc1 = adcRead(PIN_VRC1);

  gHw.thr   = mapThrottle(gHw.rawT);
  gHw.roll  = mapStick(gHw.rawR, gSet.rmin, gSet.rmax);
  gHw.pitch = mapStick(gHw.rawP, gSet.pmin, gSet.pmax);
  gHw.yaw   = mapStick(gHw.rawY, gSet.ymin, gSet.ymax);

  gHw.sw1 = (digitalRead(PIN_SW1) == LOW);
  gHw.sw2 = (digitalRead(PIN_SW2) == LOW);
  gHw.sw3 = (digitalRead(PIN_SW3) == LOW);
  gHw.sw4 = (digitalRead(PIN_SW4) == LOW);

  gHw.vrc1Id   = (uint8_t)constrain(map((long)gHw.rawVrc1, 0, 4095, 0, 255), 0, 255);
  gHw.vrc1Zone = vrc1Zone(gHw.rawVrc1);
  gPage = gHw.vrc1Zone;

  /* 偵測硬體搖桿「實際移動」(不是絕對位置), 有動作才交還硬體優先權;
   * 避免油門停在較高位置時, 軟體搖桿永遠無法接手 */
  static float prevRoll = 0, prevPitch = 0, prevYaw = 0;
  static long  prevThr  = THR_MIN_US;
  bool hwMoved = fabsf(gHw.roll  - prevRoll)  > 8 ||
                 fabsf(gHw.pitch - prevPitch) > 8 ||
                 fabsf(gHw.yaw   - prevYaw)   > 8 ||
                 labs(gHw.thr - prevThr) > 25;
  prevRoll = gHw.roll; prevPitch = gHw.pitch; prevYaw = gHw.yaw; prevThr = gHw.thr;
  if (hwMoved) gSoftMs = 0;

  /* 自動模式: 一旦使用軟體搖桿就持續由軟體控制, 直到硬體搖桿實際移動 */
  gCtrlActive = gSet.ctrlSource;
  if (gCtrlActive == CTRL_AUTO) {
    gCtrlActive = gSoftMs ? CTRL_SOFTWARE : CTRL_HARDWARE;
  }
}

/* 目前應由韌體送出的硬體命令 (軟體模式或尚未校正時不送) */
static inline bool inputHardwareActive() {
  return gSet.calValid && gCtrlActive == CTRL_HARDWARE;
}

static inline float inputRoll()  { return gCtrlActive == CTRL_SOFTWARE ? gSoftRoll  : gHw.roll; }
static inline float inputPitch() { return gCtrlActive == CTRL_SOFTWARE ? gSoftPitch : gHw.pitch; }
static inline float inputYaw()   { return gCtrlActive == CTRL_SOFTWARE ? gSoftYaw   : gHw.yaw; }
static inline long  inputThr()   { return gCtrlActive == CTRL_SOFTWARE ? gSoftThr   : gHw.thr; }

#endif