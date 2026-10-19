/*
 * KKBOX 飛控板 (KKmulticontroller v5.5, ATmega168) 韌體
 * ---------------------------------------------------------------
 * 版本 (Version): 1.0.0        (見 config.h 的 FW_VERSION)
 * 名稱 (Name):    KKBOX
 * ---------------------------------------------------------------
 * - Arduino IDE (.ino)
 * - PID 演算法 (三軸 rate PID)
 * - 自主平衡 (陀螺儀回授阻尼, 抵抗機體轉動)
 * - RS232: TX=硬體 UART (PD1/GPIO17=D1), RX=SoftwareSerial (PD2/GPIO18=D2)
 *   以 JSON 傳輸: 控制命令 + 三軸陀螺儀資料。TX 用硬體 UART 發送時
 *   不關中斷, 故遙測輸出不會屏蔽命令接收 (SoftwareSerial TX 會)。
 * - 板載 LED1 遠端遙控測試 (on / off / blink)
 *
 * 序列命令請見 README.md。所有命令以 '\n' 結尾。
 * 遙測字串以 F() 存放於 Flash, 避免佔用 ATmega168 僅 1KB 的 SRAM。
 */

#include <Servo.h>
#define _SS_MAX_RX_BUFF 128   /* SoftwareSerial RX 緩衝 (預設64B, 命令字串可達~70B 會被截斷) */
#include <SoftwareSerial.h>
#include "config.h"
#include "pid.h"
#include "gyro.h"
#include "mixer.h"
#include "jsonutil.h"

static const uint8_t MOTOR_PINS[8] = {
  MOTOR1_PIN, MOTOR2_PIN, MOTOR3_PIN, MOTOR4_PIN,
  MOTOR5_PIN, MOTOR6_PIN, MOTOR7_PIN, MOTOR8_PIN
};

Servo esc[MAX_MOTORS];
/* RX=SoftwareSerial(D2); TX 不使用 SoftwareSerial, 改走硬體 UART TXD (PD1 同腳) */
SoftwareSerial rs232(RS232_RX_PIN, RS232_TX_PIN);   /* 參數順序: RX, TX */
Gyro  gyro;
PID   pidRoll, pidPitch, pidYaw;

GyroData gyroNow;
float setRoll  = 0.0f;
float setPitch = 0.0f;
float setYaw   = 0.0f;
long  throttleUs = ESC_MIN;
bool  armed = false;
bool  hold  = false;         /* 定高 (油門保持): 鎖定當前油門 */
uint8_t motorCount = MOTOR_COUNT;

uint8_t  ledMode = 2;                 /* 0=off 1=on 2=blink */
uint8_t  ledValue = 0;
uint16_t telIntervalMs = TEL_INTERVAL_MS;

uint32_t lastCtrlUs = 0;
uint32_t lastTelMs  = 0;
uint32_t lastCmdMs  = 0;
uint32_t lastLedMs  = 0;

float motorUs[MAX_MOTORS];

/* 依馬達數掛載/卸載 Servo, 可執行中切換 4 / 8 (X8 同軸) */
static void attachMotors(uint8_t count) {
  if (count != 8) count = 4;
  for (uint8_t i = 0; i < MAX_MOTORS; i++) {
    if (i < count) {
      if (!esc[i].attached()) esc[i].attach(MOTOR_PINS[i]);
      esc[i].writeMicroseconds(ESC_MIN);
      motorUs[i] = ESC_MIN;
    } else if (esc[i].attached()) {
      esc[i].detach();
    }
  }
  motorCount = count;
}

char    lineBuf[LINE_BUF_SIZE];
uint8_t lineLen = 0;

/* ------------------------------------------------------------------ */
static void ackCmd(const char* what) {
  Serial.print(F("{\"ack\":\""));
  Serial.print(what);
  Serial.println(F("\"}"));
}

/* 固定頻率控制回路: 讀陀螺 -> PID -> 混控 -> 輸出 ESC */
static void controlStep(float dt) {
  gyroNow = gyro.read();

  float rOut = 0.0f, pOut = 0.0f, yOut = 0.0f;
  if (armed) {
    rOut = pidRoll.update(setRoll,  gyroNow.roll,  dt);
    pOut = pidPitch.update(setPitch, gyroNow.pitch, dt);
    yOut = pidYaw.update(setYaw,   gyroNow.yaw,   dt);
  } else {
    pidRoll.reset();
    pidPitch.reset();
    pidYaw.reset();
  }

  float thr = armed ? (float)throttleUs : (float)ESC_MIN;
  mixerCompute(motorCount, thr, rOut, pOut, yOut, motorUs);

  for (uint8_t i = 0; i < motorCount; i++) {
    float v = motorUs[i];
    if (v < ESC_MIN) v = ESC_MIN;
    if (v > ESC_MAX) v = ESC_MAX;
    motorUs[i] = v;
    esc[i].writeMicroseconds((int)v);
  }

  if (armed && (millis() - lastCmdMs > FAILSAFE_MS)) {
    armed = false;
    Serial.println(F("{\"evt\":\"failsafe\"}"));
  }
}

/* 非阻塞 LED 顯示: 未解鎖慢閃, 已解鎖快閃 */
static void updateLed() {
  if (ledMode == 0) {
    ledValue = 0;
  } else if (ledMode == 1) {
    ledValue = 1;
  } else {
    uint16_t period = armed ? 25 : 500;
    if (millis() - lastLedMs >= period) {
      lastLedMs = millis();
      ledValue = !ledValue;
    }
  }
  ledWrite(ledValue);
}

static void sendTelemetry() {
  if (telIntervalMs == 0) return;
  uint32_t now = millis();
  if (now - lastTelMs < telIntervalMs) return;
  /* RX 進行中先緩送 (SoftwareSerial TX 關中斷期間會屏蔽收訊),
   * 但最多延後 200ms: 避免斷行殘留 (lineLen 卡住) 令遙測永久停擺 */
  if ((rs232.available() || lineLen) && now - lastTelMs < telIntervalMs + 200) return;
  lastTelMs = now;

  Serial.print(F("{\"t\":"));               Serial.print(now);
  Serial.print(F(",\"armed\":"));           Serial.print(armed);
  Serial.print(F(",\"led\":"));             Serial.print(ledValue);
  Serial.print(F(",\"mcount\":"));          Serial.print(motorCount);
  Serial.print(F(",\"hold\":"));            Serial.print(hold);

  /* 數值一律以整數輸出: 省下 ATmega168 昂貴的浮點列印程式碼 (省下約 1KB Flash) */
  Serial.print(F(",\"gyro\":{\"roll\":"));  Serial.print((int)gyroNow.roll);
  Serial.print(F(",\"pitch\":"));           Serial.print((int)gyroNow.pitch);
  Serial.print(F(",\"yaw\":"));             Serial.print((int)gyroNow.yaw);

  Serial.print(F("},\"set\":{\"roll\":"));  Serial.print((int)setRoll);
  Serial.print(F(",\"pitch\":"));           Serial.print((int)setPitch);
  Serial.print(F(",\"yaw\":"));             Serial.print((int)setYaw);
  Serial.print(F(",\"thr\":"));             Serial.print(throttleUs);

  Serial.print(F("},\"motor\":["));
  for (uint8_t i = 0; i < motorCount; i++) {
    if (i) Serial.print(',');
    Serial.print((int)(motorUs[i] + 0.5f));
  }
  Serial.println(F("]}"));
}

static void applyPid(const char* axis, float kp, float ki, float kd) {
  PID* p = 0;
  if      (!strcmp(axis, "roll"))  p = &pidRoll;
  else if (!strcmp(axis, "pitch")) p = &pidPitch;
  else if (!strcmp(axis, "yaw"))   p = &pidYaw;
  if (p) p->setTunings(kp, ki, kd);
}

static void handleCommand(const char* s) {
  lastCmdMs = millis();

  char cmd[16];
  if (!jStr(s, "cmd", cmd, sizeof cmd)) return;

  if (!strcmp(cmd, "ping")) {
    ackCmd("ping");

  } else if (!strcmp(cmd, "led")) {
    char mode[8];
    if (jStr(s, "mode", mode, sizeof mode)) {
      if      (!strcmp(mode, "on"))    ledMode = 1;
      else if (!strcmp(mode, "off"))   ledMode = 0;
      else if (!strcmp(mode, "blink")) ledMode = 2;
    } else {
      ledMode = jInt(s, "value", 0) ? 1 : 0;
    }
    Serial.print(F("{\"ack\":\"led\",\"mode\":"));
    Serial.print(ledMode);
    Serial.println(F("}"));

  } else if (!strcmp(cmd, "arm")) {
    throttleUs = ESC_MIN;
    hold = false;
    armed = true;
    ackCmd("arm");

  } else if (!strcmp(cmd, "disarm")) {
    armed = false;
    hold = false;
    ackCmd("disarm");

  } else if (!strcmp(cmd, "calibrate")) {
    gyro.calibrate(GYRO_CAL_SAMPLES);
    ackCmd("calibrate");

  } else if (!strcmp(cmd, "throttle")) {
    long v = jInt(s, "value", throttleUs);
    if (v < ESC_MIN) v = ESC_MIN;
    if (v > ESC_MAX) v = ESC_MAX;
    if (!hold) throttleUs = v;   /* 定高中: 保持當前油門, 忽略變更 */
    ackCmd("throttle");

  } else if (!strcmp(cmd, "hold")) {
    hold = jInt(s, "on", 0) ? true : false;
    ackCmd("hold");

  } else if (!strcmp(cmd, "motors")) {
    attachMotors((uint8_t)jInt(s, "count", motorCount));
    ackCmd("motors");

  } else if (!strcmp(cmd, "setpoints")) {
    setRoll  = jFloat(s, "roll",  setRoll);
    setPitch = jFloat(s, "pitch", setPitch);
    setYaw   = jFloat(s, "yaw",   setYaw);
    ackCmd("setpoints");

  } else if (!strcmp(cmd, "pid")) {
    char axis[8] = "roll";
    jStr(s, "axis", axis, sizeof axis);
    applyPid(axis, jFloat(s, "kp", 0), jFloat(s, "ki", 0), jFloat(s, "kd", 0));
    ackCmd("pid");

  } else if (!strcmp(cmd, "telemetry")) {
    long v = jInt(s, "value", telIntervalMs);
    if (v < 0) v = 0;
    if (v > 1000) v = 1000;
    telIntervalMs = (uint16_t)v;
    ackCmd("telemetry");

  } else if (!strcmp(cmd, "status")) {
    Serial.print(F("{\"ack\":\"status\",\"fw\":\"" FW_NAME "\",\"ver\":\"" FW_VERSION "\",\"armed\":"));
    Serial.print(armed);
    Serial.print(F(",\"thr\":"));
    Serial.print(throttleUs);
    Serial.print(F(",\"tel\":"));
    Serial.print(telIntervalMs);
    Serial.print(F(",\"mcount\":"));
    Serial.print(motorCount);
    Serial.print(F(",\"hold\":"));
    Serial.print(hold);
    Serial.println(F("}"));

  } else {
    Serial.println(F("{\"err\":\"unknown cmd\"}"));
  }
}

static void readSerial() {
  while (rs232.available()) {
    char c = (char)rs232.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        lineBuf[lineLen] = 0;
        handleCommand(lineBuf);
        lineLen = 0;
      }
    } else if (lineLen < LINE_BUF_SIZE - 1) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;   /* 溢位: 丟棄此行 */
    }
  }
}

void setup() {
  /* TX: 硬體 UART (PD1/GPIO17) — UDRE 中斷驅動, 發送期間不關中斷 */
  Serial.begin(SERIAL_BAUD);
  UCSR0B &= ~_BV(RXCIE0);   /* 停用硬體 RX 中斷: D0 已釋出懸空, 免雜訊觸發 */
  PORTD |= _BV(PD0);        /* D0 內部上拉, 維持 idle high */
  /* RX: SoftwareSerial (D2/GPIO18) */
  rs232.begin(SERIAL_BAUD);
  readSerial();             /* 清掉開機瞬間的殘留位元組 */
  ledInit();

  attachMotors(MOTOR_COUNT);

  pidRoll.setTunings(PID_ROLL_KP, PID_ROLL_KI, PID_ROLL_KD);
  pidPitch.setTunings(PID_PITCH_KP, PID_PITCH_KI, PID_PITCH_KD);
  pidYaw.setTunings(PID_YAW_KP, PID_YAW_KI, PID_YAW_KD);
  pidRoll.setOutputLimits(-PID_OUT_LIMIT, PID_OUT_LIMIT);
  pidPitch.setOutputLimits(-PID_OUT_LIMIT, PID_OUT_LIMIT);
  pidYaw.setOutputLimits(-PID_OUT_LIMIT, PID_OUT_LIMIT);
  pidRoll.setIntegralLimit(PID_I_LIMIT);
  pidPitch.setIntegralLimit(PID_I_LIMIT);
  pidYaw.setIntegralLimit(PID_I_LIMIT);

  gyro.calibrate(GYRO_CAL_SAMPLES);

  gyroNow.roll = gyroNow.pitch = gyroNow.yaw = 0.0f;
  lastCtrlUs = micros();
  lastCmdMs  = millis();
  lastTelMs  = millis();

  Serial.print(F("{\"evt\":\"ready\",\"fw\":\"" FW_NAME "\",\"ver\":\"" FW_VERSION "\"}"));
  Serial.println();
}

void loop() {
  readSerial();

  uint32_t nowUs = micros();
  uint32_t durUs = nowUs - lastCtrlUs;
  if (durUs >= CTRL_PERIOD_US) {
    lastCtrlUs = nowUs;
    controlStep(durUs * 1e-6f);
  }

  updateLed();
  sendTelemetry();
}
