#ifndef KKBOX_GYRO_H
#define KKBOX_GYRO_H

#include <Arduino.h>
#include "config.h"

struct GyroData {
  float roll;
  float pitch;
  float yaw;
};

/* 一維卡爾曼濾波 (隨機漫步模型): 單軸角速度去噪。
 * 預測: x = x, P = P + Q
 * 更新: K = P / (P + R), x += K * (z - x), P = (1 - K) * P
 * Q 大 -> 信任量測、反應快; R 大 -> 信任模型、較平滑。
 */
class Kalman1D {
public:
  Kalman1D() : _x(0), _p(0) {}
  void reset() { _x = 0; _p = 0; }
  float update(float z) {
    if (_p <= 0.0f) {          /* 第一筆: 直接以量測初始化 */
      _x = z;
      _p = GYRO_KALMAN_P_INIT;
      return _x;
    }
    _p += GYRO_KALMAN_Q;
    float k = _p / (_p + GYRO_KALMAN_R);
    _x += k * (z - _x);
    _p *= (1.0f - k);
    return _x;
  }

private:
  float _x, _p;
};

/* 壓電陀螺儀讀取器:
 * 1. 開機 / calibrate() 時量測靜止零點偏移。
 * 2. read() 扣掉零點、換算為 deg/s, 再做卡爾曼濾波
 *    (GYRO_KALMAN_ENABLE=0 時改回一階低通 GYRO_LPF)。
 */
class Gyro {
public:
  Gyro() : _oR(0), _oP(0), _oY(0), _fR(0), _fP(0), _fY(0) {}

  void calibrate(uint16_t samples) {
    if (samples == 0) samples = 1;
    long sR = 0, sP = 0, sY = 0;
    for (uint16_t i = 0; i < samples; i++) {
      sR += analogRead(GYRO_ROLL_PIN);
      sP += analogRead(GYRO_PITCH_PIN);
      sY += analogRead(GYRO_YAW_PIN);
      delay(2);
    }
    _oR = (float)sR / samples;
    _oP = (float)sP / samples;
    _oY = (float)sY / samples;
    _fR = _fP = _fY = 0;
#if GYRO_KALMAN_ENABLE
    _kR.reset(); _kP.reset(); _kY.reset();
#endif
  }

  GyroData read() {
    float r = (analogRead(GYRO_ROLL_PIN)  - _oR) * GYRO_DEG_PER_COUNT;
    float p = (analogRead(GYRO_PITCH_PIN) - _oP) * GYRO_DEG_PER_COUNT;
    float y = (analogRead(GYRO_YAW_PIN)   - _oY) * GYRO_DEG_PER_COUNT;

#if GYRO_REVERSE_ROLL
    r = -r;
#endif
#if GYRO_REVERSE_PITCH
    p = -p;
#endif
#if GYRO_REVERSE_YAW
    y = -y;
#endif

#if GYRO_KALMAN_ENABLE
    _fR = _kR.update(r);
    _fP = _kP.update(p);
    _fY = _kY.update(y);
#else
    _fR += GYRO_LPF * (r - _fR);
    _fP += GYRO_LPF * (p - _fP);
    _fY += GYRO_LPF * (y - _fY);
#endif

    GyroData d = { _fR, _fP, _fY };
    return d;
  }

  float offsetRoll()  const { return _oR; }
  float offsetPitch() const { return _oP; }
  float offsetYaw()   const { return _oY; }

private:
  float _oR, _oP, _oY;
  float _fR, _fP, _fY;
#if GYRO_KALMAN_ENABLE
  Kalman1D _kR, _kP, _kY;
#endif
};

#endif
