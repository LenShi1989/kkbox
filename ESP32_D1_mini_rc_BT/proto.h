#ifndef RC_BT_PROTO_H
#define RC_BT_PROTO_H

/* =============================================================
 * 16-byte 二進位協定 (沿用測試碼 esp32_server---TESTOK-----20250710)
 * -------------------------------------------------------------
 * TX (遙控器 -> 無人機)  16 bytes:
 *   [0] throttle  [1] yaw      [2] pitch    [3] roll      (各 0~255)
 *   [4] vrc1      [5] sw1      [6] sw2      [7] sw3       (sw: 0=ON, 1=OFF)
 *   [8] sw4       [9] vrc2    [10] vrc3    [11] vrc4      (vrc: 0~255)
 *   [12] 遙控器編號 (rcId, 0~255)
 *   [13] 目標無人機編號 (flyId, 0~255)
 *   [14] 未定義
 *   [15] 1 = 有新資料
 *
 * RX (無人機 -> 遙控器)  16 bytes:
 *   [0..7]   Motor1~8 PWM (0~255)
 *   [8] pitch  [9] roll  [10] yaw  [11] temp (degC)
 *   [12] height high byte  [13] height low byte  (+-32768 m)
 *   [14] 無人機 id  [15] 1 = 有新資料
 * ============================================================= */

#include <Arduino.h>
#include "config.h"
#include "settings.h"

static uint8_t gTx[16] = {0};
static uint8_t gRx[16] = {0};
static bool    gRxNew = false;
static bool    gBtLinked = false;
static uint32_t gTxCount = 0, gRxCount = 0;

static inline int clampi(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

/* ADC 原始值 -> 0~255 (依校正 min/max) */
static inline uint8_t adcToByte(int v, int vmin, int vmax) {
  if (vmax <= vmin) return 128;
  return (uint8_t)clampi(map(v, vmin, vmax, 0, 255));
}

/* 讀取搖桿/開關並打包成 TX 16 bytes */
static void protoPack() {
  gTx[0] = adcToByte(analogRead(PIN_THROTTLE), gSet.tmin, gSet.tmax);
  gTx[1] = adcToByte(analogRead(PIN_YAW),      gSet.ymin, gSet.ymax);
  gTx[2] = adcToByte(analogRead(PIN_PITCH),    gSet.pmin, gSet.pmax);
  gTx[3] = adcToByte(analogRead(PIN_ROLL),     gSet.rmin, gSet.rmax);
  gTx[4] = adcToByte(analogRead(PIN_VRC1), 0, 4095);
  gTx[5] = digitalRead(PIN_SW1) == LOW ? 0 : 1;
  gTx[6] = digitalRead(PIN_SW2) == LOW ? 0 : 1;
  gTx[7] = digitalRead(PIN_SW3) == LOW ? 0 : 1;
  gTx[8] = digitalRead(PIN_SW4) == LOW ? 0 : 1;
  gTx[9]  = adcToByte(analogRead(PIN_VRC2), 0, 4095);
  gTx[10] = adcToByte(analogRead(PIN_VRC3), 0, 4095);
  gTx[11] = adcToByte(analogRead(PIN_VRC4), 0, 4095);
  gTx[12] = gSet.rcId;
  gTx[13] = gSet.flyId;
  gTx[14] = 1;
  gTx[15] = 1;          /* 1 = 有新資料 */
}

#endif
