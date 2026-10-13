# kkbox

## KK飛控板 V5.5 採用 Atmel Atmega168晶片

---

| 外部端子 / 模組標示 | GPIO編號 | Pin 腳編號 | 功能與說明                                         |
| ------------------- | -------- | ---------- | -------------------------------------------------- |
| VCC                 | N/A      | N/A        | DC 5V                                              |
| GND                 | N/A      | N/A        | 接地                                               |
| RS232-RX            | 17       | 31         | RS232 RX                                           |
| RS232-TX            | 18       | 32         | RS232 TX                                           |
| Roll Gain           | 11       | 26         | 滾轉軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益） |
| Pitch Gain          | 12       | 27         | 俯仰軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益） |
| Yaw Gain            | 13       | 28         | 偏航軸陀螺儀類比訊號輸入（經可變電阻調整增益）     |
| 三軸陀螺儀          | 8        | 23         | X+Y+Z，共三顆                                      |
| Pitch 陀螺儀        | 9        | 24         | Gyro pitch                                         |
| Roll 陀螺儀         | 10       | 25         | Gyro roll                                          |
| Motor 1 (M1)        | 2        | 14         | 第 1 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 2 (M2)        | 1        | 13         | 第 2 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 3 (M3)        | 0        | 12         | 第 3 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 4 (M4)        | 23       | 11         | 第 4 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 5 (M5)        | 22       | 10         | 第 5 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 6 (M6)        | 21       | 9          | 第 6 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 7 (M7)        | 19       | 1          | 第 7 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 8 (M8)        | 7        | 8          | 第 8 號馬達 / 電子調速器（ESC）控制輸出            |
| LED1                | 6        | 7          | 板載LED燈                                          |

---

## 專案結構

```
kkbox/
├── README.md                     專案說明
├── docs/                         硬體參考資料
│   ├── KK飛控板-1.png
│   ├── KK飛控板-2.png
│   ├── KK飛控板-3.png
│   ├── KK飛控板-4.png
│   ├── KKBOX原理圖-1.jpg
│   ├── KKBOX原理圖-2.jpg
│   ├── KKBOX接腳定義.xlsx
│   └── KKmulticontroller_v5.5.pdf
├── firmware/
│   └── kkbox/                    Arduino 韌體 (IDE 開啟 kkbox.ino)
│       ├── kkbox.ino             主程式: 控制回路 + 序列 JSON 協定
│       ├── config.h              腳位對照與可調參數
│       ├── pid.h                 PID 演算法
│       ├── gyro.h                三軸陀螺儀讀取 / 校正 / 濾波
│       ├── mixer.h               四軸混控 (自主平衡輸出)
│       └── jsonutil.h            輕量 JSON 解析 (無動態記憶體)
└── tools/
    └── kkbox_monitor.py          電腦端序列埠遠端測試工具 (pyserial)
```

## 開發環境

- Arduino IDE, 開啟 `firmware/kkbox/kkbox.ino`
- 開發板: `Arduino Pro or Pro Mini` → `ATmega168, 8MHz` (本板依 docs 使用內建 8MHz RC 振盪器)
- 燒錄: 透過 ISP (AVRISP/Arduino as ISP)，或先燒 bootloader 後用 USB-TTL 走 RS232

## Arduino 腳位對照

GPIO 編號 → ATmega168 埠 → Arduino 腳位（詳見 `firmware/kkbox/config.h`）：

| 功能 | GPIO | 埠 | Arduino | 說明 |
| --- | --- | --- | --- | --- |
| RS232 RX | 17 | PD0 | D0 | 硬體 UART RXD |
| RS232 TX | 18 | PD1 | D1 | 硬體 UART TXD |
| 陀螺儀 Roll | 8 | ADC0 | A0 | 壓電陀螺儀 |
| 陀螺儀 Pitch | 9 | ADC1 | A1 | 壓電陀螺儀 |
| 陀螺儀 Yaw | 10 | ADC2 | A2 | 壓電陀螺儀 |
| Roll/Pitch/Yaw Gain | 11/12/13 | ADC3/4/5 | A3/A4/A5 | 可變電阻（預設停用） |
| M1 / M2 / M3 | 2/1/0 | PB2/PB1/PB0 | D10/D9/D8 | ESC 輸出 |
| M4 / M5 / M6 | 23/22/21 | PD6/PD5/PD4 | D6/D5/D4 | ESC 輸出 |
| M7 | 19 | PD2 | D2 | ESC 輸出 |
| M8 | 7 | PB7 | (XTAL2) | 僅內建振盪器時可用 |
| LED1 | 6 | PB6 | (XTAL1) | 板載 LED，用直接埠控制 |

> 注意：LED1 (PB6) 與 M8 (PB7) 是標準 Arduino 的晶體腳。本板使用內建振盪器才可當 GPIO；
> 若外接晶體，請在 `config.h` 將 `LED1_DIRECT` 設為 0 並改用其他腳位。

## 序列埠 JSON 協定

鮑率 **57600 8N1**，每行一筆 JSON，以 `\n` 結尾。

### 電腦 → 飛控板（控制命令）

```json
{"cmd":"ping"}
{"cmd":"status"}
{"cmd":"led","mode":"on"}          // on / off / blink，或 {"cmd":"led","value":1}
{"cmd":"arm"}                       // 解鎖後油門歸零, 開始 PID 平衡
{"cmd":"disarm"}                    // 上鎖, 馬達停止
{"cmd":"calibrate"}                 // 陀螺儀零點校正 (機體需靜止)
{"cmd":"throttle","value":1300}     // 油門 1000~2000 us
{"cmd":"setpoints","roll":0,"pitch":0,"yaw":0}   // 角速率目標 (deg/s)
{"cmd":"pid","axis":"roll","kp":3.0,"ki":0.0,"kd":8.0}
{"cmd":"telemetry","value":50}      // 遙測週期 ms, 0 = 關閉
```

### 飛控板 → 電腦（遙測，含三軸陀螺儀資料）

```json
{"t":12345,"armed":1,"led":0,
 "gyro":{"roll":1.23,"pitch":-4.56,"yaw":0.78},
 "set":{"roll":0.00,"pitch":0.00,"yaw":0.00,"thr":1300},
 "motor":[1420,1380,1460,1350]}
```

回應命令時回傳 `{"ack":"..."}`；解鎖狀態下若 2 秒內未收到任何命令會回報 `{"evt":"failsafe"}` 並自動上鎖。

## 遠端控制板載 LED1 測試

```powershell
python tools/kkbox_monitor.py --port COM3 --led blink
python tools/kkbox_monitor.py --port COM3 --led on
python tools/kkbox_monitor.py --port COM3 --led off
```

互動模式：

```powershell
python tools/kkbox_monitor.py --port COM3
> led blink
> arm
> thr 1300
> sp 0 0 0
> q
```

未解鎖時 LED 慢閃（2Hz），解鎖後快閃（20Hz）。

## PID 調校

三軸為 **rate PID**（陀螺儀角速度回授），即 KK 板的「自主平衡」阻尼。調校順序：

1. 固定機體、`cal` 校正後 `arm`，用手輕推機體觀察抵抗方向是否正確；方向反了改 `config.h` 的 `GYRO_REVERSE_*`。
2. 先加 `kp` 直到開始快速震盪，再退回一點。
3. 加 `kd` 抑制震盪；yaw 軸可少量 `ki` 做定角速率。
4. `GYRO_DEG_PER_COUNT` 依實際陀螺儀靈敏度校正，會影響 PID 增益尺度。
