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
/* 9600: 8MHz SoftwareSerial RX 實測只能可靠收到 9600 (設 57600 時飛控收不到命令)。
 * 8MHz 下 57600 每位元僅 ~139 週期, 接收取樣跟不上; 9600 則有充裕餘裕。
 * TX 已改硬體 UART (UDRE 中斷驅動, 發送不關中斷也不阻塞), 故低速不再妨礙命令接收。
 * (115200 為原始設計值, 8MHz SoftwareSerial 無法勝任) */
#define SERIAL_BAUD 9600

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
 * 開機預設 4 槳 (MOTOR_COUNT=4), 可執行中以命令切換為 8 槳 (X8 同軸)。
 * GPIO 對照: M1=GPIO2  M2=GPIO1  M3=GPIO0  M4=GPIO23
 *             M5=GPIO22 M6=GPIO21 M7=GPIO19 M8=GPIO20
 * 轉 Arduino pin: M1=D10(PB2) M2=D9(PB1) M3=D8(PB0)
 *                 M4=D7(PD7)  M5=D6(PD6) M6=D5(PD5) M7=D3(PD3) M8=D4(PD4)
 * 註: 原 docs 的 M8=GPIO7(PB7/XTAL2) 於本板無法當 GPIO, 故 M8 改接 D4。
 */
#define MOTOR_COUNT 4        /* 開機預設馬達數 (4 或 8) */
#define MAX_MOTORS  8        /* Servo 陣列上限 (勿超過 Servo 函式庫上限) */
#define MOTOR1_PIN 10
#define MOTOR2_PIN  9
#define MOTOR3_PIN  8
#define MOTOR4_PIN  7
#define MOTOR5_PIN  6
#define MOTOR6_PIN  5
#define MOTOR7_PIN  3
#define MOTOR8_PIN  4

#define ESC_MIN 1000
#define ESC_MAX 2000

/* ---------- 機架設定 ---------- */
#define KK_FRAME 0   /* 0 = Quad-X, 1 = Quad-+ */

/* ---------- 控制回路 ---------- */
/* 控制迴路執行頻率 = 陀螺取樣 / PID 更新 / 馬達 PWM 輸出更新頻率。
 * 越高反應越快但越吃 8MHz MCU 時間; 越低馬達轉速變動越慢越柔。
 * 200Hz->100Hz: 更新週期由 5ms 變 10ms, 轉速變動頻率減半 (反應較柔, 穩定性可能下降)。 */
#define CTRL_HZ        100      /* PID 更新頻率 100Hz, 10ms */
#define CTRL_PERIOD_US (1000000UL / CTRL_HZ)
#define FAILSAFE_MS    2000

/* ---------- 陀螺儀校正 ---------- */
#define GYRO_CAL_SAMPLES    400
#define GYRO_DEG_PER_COUNT  0.6f   /* deg/s per ADC count, 請依實測調整 */
#define GYRO_LPF            0.6f   /* 一階低通係數 (僅 GYRO_KALMAN_ENABLE=0 時使用) */

/* ---------- 陀螺儀卡爾曼濾波 (一維/軸) ----------
 * 對三軸角速度各做隨機漫步模型的純量卡爾曼濾波:
 *   預測: x = x, P = P + Q
 *   更新: K = P / (P + R), x += K * (z - x), P = (1 - K) * P
 * Q 越大越信任新量測 (反應快、抖動多), R 越大越信任模型 (平滑、延遲大)。
 * 100Hz 下 Q=2.0 / R=2.0 的穩態增益約 0.62, 時間常數約 10ms,
 * 與原本 GYRO_LPF=0.6 (約 11ms) 手感接近, 故既有 PID 預設值可直接沿用。
 * 振動大 -> 降 Q 或升 R (更平滑、延遲增加); 反應鈍 -> 升 Q 或降 R。
 * 將 GYRO_KALMAN_ENABLE 設為 0 即恢復原本的 GYRO_LPF 一階低通。 */
#define GYRO_KALMAN_ENABLE 1
#define GYRO_KALMAN_Q      2.0f
#define GYRO_KALMAN_R      2.0f
#define GYRO_KALMAN_P_INIT 1.0f

/* ---------- PID 預設 ---------- */
/* PID 修正量上限 (us): roll/pitch/yaw 各自加在油門上的修正會被夾在此範圍內。
 * 越小修正幅度越小、動作越柔和; 300us->100us 縮為原本 1/3。 */
#define PID_OUT_LIMIT 100.0f  /* PID 變動幅度上限 100us */
#define PID_I_LIMIT   50.0f
#define PID_ROLL_KP   3.0f
#define PID_ROLL_KI   0.08f   /* 自動配平 (auto-trim), 見下方 TRIM_DEADBAND */
#define PID_ROLL_KD   8.0f
#define PID_PITCH_KP  3.0f
#define PID_PITCH_KI  0.08f   /* 自動配平 (auto-trim) */
#define PID_PITCH_KD  8.0f
#define PID_YAW_KP    4.0f
#define PID_YAW_KI    0.5f
#define PID_YAW_KD    0.0f

/* ---------- 自動配平 (auto-trim) ----------
 * 依三軸陀螺儀狀態自動修正平衡: roll/pitch 的積分項 (Ki) 只在搖桿置中時累積,
 * 依陀螺殘餘轉速緩慢學習配平量, 抵銷機體/ESC 的固定偏壓, 讓機體不再持續漂移;
 * 搖桿離開中央即凍結積分 (保留已學習的配平值)。積分上限 = PID_I_LIMIT。 */
#define TRIM_DEADBAND 10.0f   /* 搖桿命令絕對值小於此值視為置中 (deg/s) */

/* ---------- 序列緩衝 (ATmega168 僅 1KB SRAM, 請勿任意放大) ---------- */
#define LINE_BUF_SIZE   72
/* 遙測週期 (ms), 0 = 關閉。9600 鮑率下一幀約 160 位元組 ≈ 167ms 傳輸時間;
 * 設 200 保留約 2 成餘裕, 避免 Serial.print 因 TX 緩衝滿而長期阻塞、拖慢 PID 控制週期。
 * TX 阻塞期間 RX 仍由硬體中斷收進緩衝, 不影響命令接收。 */
#define TEL_INTERVAL_MS 200

#endif
