#ifndef KKBOX_MIXER_H
#define KKBOX_MIXER_H

#include <Arduino.h>
#include "config.h"

/* 混控: 輸出 (us) = 油門 + roll/pitch/yaw PID 修正量。
 * 支援執行中切換 4 槳 (Quad-X / Quad-+) 與 8 槳 (X8 同軸)。
 *
 * 4 槳 Quad-X (機頭朝上):
 *       M4(FL)        M1(FR)
 *            \  X  /
 *       M3(RL)        M2(RR)
 *
 * 8 槳 X8 同軸 (4 臂, 每臂上下兩顆反槳; 上槳 M1..M4, 下槳 M5..M8):
 *   臂位與 4 槳相同; 同臂上下同 roll/pitch, yaw 反號 (反槳)。
 */

/* 每臂係數 [roll, pitch, yaw] (單一槳); int8 省 Flash */
#if KK_FRAME == 0
static const int8_t MIX_ARM[4][3] = {
  { -1, -1,  1 },  /* M1 FR */
  {  1, -1, -1 },  /* M2 RR */
  {  1,  1,  1 },  /* M3 RL */
  { -1,  1, -1 }   /* M4 FL */
};
#else
static const int8_t MIX_ARM[4][3] = {
  {  0, -1,  1 },  /* M1 前 */
  { -1,  0, -1 },  /* M2 右 */
  {  0,  1,  1 },  /* M3 後 */
  {  1,  0, -1 }   /* M4 左 */
};
#endif

/* 依 motorCount (4 或 8) 計算各馬達輸出 (us) */
static void mixerCompute(uint8_t motorCount, float thr,
                         float roll, float pitch, float yaw, float* out) {
  uint8_t n = (motorCount == 8) ? 8 : 4;
  for (uint8_t i = 0; i < n; i++) {
    uint8_t a = i & 3;
    float y = (i < 4) ? yaw : -yaw;   /* 同軸下槳 yaw 反號 */
    out[i] = thr + MIX_ARM[a][0] * roll + MIX_ARM[a][1] * pitch + MIX_ARM[a][2] * y;
  }
}

#endif
