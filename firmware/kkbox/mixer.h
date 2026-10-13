#ifndef KKBOX_MIXER_H
#define KKBOX_MIXER_H

#include <Arduino.h>
#include "config.h"

/* 四軸混控; 輸出 (us) = 油門 + roll/pitch/yaw PID 修正量。
 *
 * Quad-X 馬達配置 (機頭朝上):
 *       M4(FL)        M1(FR)
 *            \  X  /
 *       M3(RL)        M2(RR)
 *
 * Quad-+ 馬達配置:
 *            M1(前)
 *       M4(左)    M2(右)
 *            M3(後)
 */
#if MOTOR_COUNT != 4
#error "mixer.h 目前僅支援 4 顆馬達 (MOTOR_COUNT == 4)"
#endif

static void mixerCompute(float thr, float roll, float pitch, float yaw,
                         float out[MOTOR_COUNT]) {
#if KK_FRAME == 0
  static const float c[MOTOR_COUNT][3] = {
    { -1.0f, -1.0f,  1.0f },  /* M1 FR */
    {  1.0f, -1.0f, -1.0f },  /* M2 RR */
    {  1.0f,  1.0f,  1.0f },  /* M3 RL */
    { -1.0f,  1.0f, -1.0f }   /* M4 FL */
  };
#else
  static const float c[MOTOR_COUNT][3] = {
    {  0.0f, -1.0f,  1.0f },  /* M1 前 */
    { -1.0f,  0.0f, -1.0f },  /* M2 右 */
    {  0.0f,  1.0f,  1.0f },  /* M3 後 */
    {  1.0f,  0.0f, -1.0f }   /* M4 左 */
  };
#endif

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    out[i] = thr + c[i][0] * roll + c[i][1] * pitch + c[i][2] * yaw;
  }
}

#endif
