#ifndef KKBOX_CONFIG_H
#define KKBOX_CONFIG_H

#include <Arduino.h>

#define FW_NAME    "KKBOX"
#define FW_VERSION "1.0.0"

/* ---------- RS232 (TX=硬體 UART, RX=SoftwareSerial) ----------
 * 接腳: RS232-TX = PD1 = GPIO17 = Arduino D1  (硬體 UART TXD, 同腳)
 *       RS232-RX = PD2 = GPIO18 = Arduino D2  (SoftwareSerial)
 * TX 走硬體 UART: 發送由 UDRE 中斷驅動、不關中斷, 遙測發送期間仍可
 * 接收命令; SoftwareSerial 發送逐字元 cli 會把 RX 屏蔽掉, 這正是
 * 命令時好時壞/延遲收到的主因。硬體 UART 的 D0/RXD 已釋出並停用。
 */
#define RS232_RX_PIN 2   /* GPIO18 = PD2 = D2 (SoftwareSerial) */
#define RS232_TX_PIN 1   /* GPIO17 = PD1 = D1 (硬體 UART TXD) */
/* 57600: 8MHz SoftwareSerial RX 的可靠上限 (139 週期/位元);
 * 9600 雖位元更寬, 但遙測一幀要 172ms, 發送佔滿線路反而讓 RX 幾乎全盲。
 * 硬體 UART TX 於 57600 實測誤差約 2%, 可用 (原設計 115200 誤差 3.5%)。 */
#define SERIAL_BAUD 57600

/* ---------- 板載 LED1 ----------
 * 原理圖 LED1 = ATmega168 PB6 (GPIO6)。
 * 注意: PB6 在標準 Arduino 板為 XTAL1; 本板依 docs 使用內建 8MHz RC
 *       振盪器, 故 PB6/PB7 可當一般 GPIO。
 * 若你的板子外接石英晶體, 請把 LED1_DIRECT 設 0 並改用 LED1_PIN。
 */
#define LED1_DIRECT 1
#if LED1_DIRECT
  #define LED1_PORT PORTB
  #define LED1_DDR  DDRB
  #define LED1_BITN PB6
#else
  #define LED1_PIN  13
#endif

static inline void ledInit() {
#if LED1_DIRECT
  LED1_DDR |= _BV(LED1_BITN);
#else
  pinMode(LED1_PIN, OUTPUT);
#endif
}

static inline void ledWrite(uint8_t on) {
#if LED1_DIRECT
  if (on) LED1_PORT |= _BV(LED1_BITN);
  else    LED1_PORT &= ~_BV(LED1_BITN);
#else
  digitalWrite(LED1_PIN, on ? HIGH : LOW);
#endif
}

/* ---------- 陀螺儀 (壓電式, 接 ADC) ----------
 * Yaw   = GPIO8  = PC0 = A0
 * Pitch = GPIO9  = PC1 = A1
 * Roll  = GPIO10 = PC2 = A2
 */
#define GYRO_ROLL_PIN   A2
#define GYRO_PITCH_PIN  A1
#define GYRO_YAW_PIN    A0

#define GYRO_REVERSE_ROLL  0
#define GYRO_REVERSE_PITCH 0
#define GYRO_REVERSE_YAW   0

/* ---------- 增益可變電阻 GPIO11/12/13 = PC3/PC4/PC5 = A3/A4/A5 ----------
 * 預設關閉; 若板上確實有可變電阻再接, 否則浮空讀值會造成增益亂跳。
 */
#define USE_GAIN_POTS  0
#define GAIN_ROLL_PIN  A3
#define GAIN_PITCH_PIN A4
#define GAIN_YAW_PIN   A5

/* ---------- 馬達 / ESC 輸出 ----------
 * GPIO 對照: M1=GPIO2  M2=GPIO1  M3=GPIO0  M4=GPIO23
 *             M5=GPIO22 M6=GPIO21 M7=GPIO19 M8=GPIO7
 * 轉 Arduino pin: M1=D10(PB2) M2=D9(PB1) M3=D8(PB0)
 *                 M4=D7(PD7)  M5=D6(PD6) M6=D5(PD5) M7=D3(PD3)
 */
#define MOTOR_COUNT 4
#define MOTOR1_PIN 10
#define MOTOR2_PIN  9
#define MOTOR3_PIN  8
#define MOTOR4_PIN  7
#define MOTOR5_PIN  6
#define MOTOR6_PIN  5
#define MOTOR7_PIN  3
#define MOTOR8_PIN  7   /* M8=GPIO7=PB7, Arduino 無對應腳位(需直接埠操作); MOTOR_COUNT=4 時未使用 */

#define ESC_MIN 1000
#define ESC_MAX 2000

/* ---------- 機架設定 ---------- */
#define KK_FRAME 0   /* 0 = Quad-X, 1 = Quad-+ */

/* ---------- 控制回路 ---------- */
#define CTRL_HZ        200
#define CTRL_PERIOD_US (1000000UL / CTRL_HZ)
#define FAILSAFE_MS    2000

/* ---------- 陀螺儀校正 ---------- */
#define GYRO_CAL_SAMPLES    400
#define GYRO_DEG_PER_COUNT  0.6f   /* deg/s per ADC count, 請依實測調整 */
#define GYRO_LPF            0.6f   /* 低通濾波係數 0~1 */

/* ---------- PID 預設 ---------- */
#define PID_OUT_LIMIT 300.0f
#define PID_I_LIMIT   50.0f
#define PID_ROLL_KP   3.0f
#define PID_ROLL_KI   0.0f
#define PID_ROLL_KD   8.0f
#define PID_PITCH_KP  3.0f
#define PID_PITCH_KI  0.0f
#define PID_PITCH_KD  8.0f
#define PID_YAW_KP    4.0f
#define PID_YAW_KI    0.5f
#define PID_YAW_KD    0.0f

/* ---------- 序列緩衝 (ATmega168 僅 1KB SRAM, 請勿任意放大) ---------- */
#define LINE_BUF_SIZE   72
/* 遙測 100ms: 57600 bps 下一幀 ~29ms; 50ms 間隔時 TX 佔空比 57%,
 * SoftwareSerial TX 期間 RX 幾乎全盲, 命令易被丟; 100ms 降至 ~29% */
#define TEL_INTERVAL_MS 100

#endif
