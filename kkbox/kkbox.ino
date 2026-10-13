/*
 * KKBOX 飛控板 (KKmulticontroller v5.5, ATmega168) 韌體
 * ---------------------------------------------------------------
 * 版本 (Version): 1.0.0        (見 config.h 的 FW_VERSION)
 * 名稱 (Name):    KKBOX
 * ---------------------------------------------------------------
 * - Arduino IDE (.ino)
 * - PID 演算法 (三軸 rate PID)
 * - 自主平衡 (陀螺儀回授阻尼, 抵抗機體轉動)
 * - RS232 TX/RX (硬體 UART) 以 JSON 傳輸: 控制命令 + 三軸陀螺儀資料
 * - 板載 LED1 遠端遙控測試 (on / off / blink)
 *
 * 序列命令請見 README.md。所有命令以 '\n' 結尾。
 * 遙測字串以 F() 存放於 Flash, 避免佔用 ATmega168 僅 1KB 的 SRAM。
 */

#include <Servo.h>
#include "config.h"
#include "pid.h"
#include "gyro.h"
#include "mixer.h"
#include "jsonutil.h"

static const uint8_t MOTOR_PINS[8] = {
  MOTOR1_PIN, MOTOR2_PIN, MOTOR3_PIN, MOTOR4_PIN,
  MOTOR5_PIN, MOTOR6_PIN, MOTOR7_PIN, MOTOR8_PIN
};

Servo esc[MOTOR_COUNT];
Gyro  gyro;
PID   pidRoll, pidPitch, pidYaw;

GyroData gyroNow;
float setRoll  = 0.0f;
float setPitch = 0.0f;
float setYaw   = 0.0f;
long  throttleUs = ESC_MIN;
bool  armed = false;

uint8_t  ledMode = 2;                 /* 0=off 1=on 2=blink */
uint8_t  ledValue = 0;
uint16_t telIntervalMs = TEL_INTERVAL_MS;

uint32_t lastCtrlUs = 0;
uint32_t lastTelMs  = 0;
uint32_t lastCmdMs  = 0;
uint32_t lastLedMs  = 0;

float motorUs[MOTOR_COUNT];

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
  mixerCompute(thr, rOut, pOut, yOut, motorUs);

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
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
  lastTelMs = now;

  Serial.print(F("{\"t\":"));               Serial.print(now);
  Serial.print(F(",\"armed\":"));           Serial.print(armed);
  Serial.print(F(",\"led\":"));             Serial.print(ledValue);

  Serial.print(F(",\"gyro\":{\"roll\":"));  Serial.print(gyroNow.roll, 2);
  Serial.print(F(",\"pitch\":"));           Serial.print(gyroNow.pitch, 2);
  Serial.print(F(",\"yaw\":"));             Serial.print(gyroNow.yaw, 2);

  Serial.print(F("},\"set\":{\"roll\":"));  Serial.print(setRoll, 2);
  Serial.print(F(",\"pitch\":"));           Serial.print(setPitch, 2);
  Serial.print(F(",\"yaw\":"));             Serial.print(setYaw, 2);
  Serial.print(F(",\"thr\":"));             Serial.print(throttleUs);

  Serial.print(F("},\"motor\":["));
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
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
    armed = true;
    ackCmd("arm");

  } else if (!strcmp(cmd, "disarm")) {
    armed = false;
    ackCmd("disarm");

  } else if (!strcmp(cmd, "calibrate")) {
    gyro.calibrate(GYRO_CAL_SAMPLES);
    ackCmd("calibrate");

  } else if (!strcmp(cmd, "throttle")) {
    long v = jInt(s, "value", throttleUs);
    if (v < ESC_MIN) v = ESC_MIN;
    if (v > ESC_MAX) v = ESC_MAX;
    throttleUs = v;
    ackCmd("throttle");

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
    Serial.println(F("}"));

  } else {
    Serial.println(F("{\"err\":\"unknown cmd\"}"));
  }
}

static void readSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
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
  Serial.begin(SERIAL_BAUD);
  ledInit();

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    esc[i].attach(MOTOR_PINS[i]);
    esc[i].writeMicroseconds(ESC_MIN);
    motorUs[i] = ESC_MIN;
  }

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
