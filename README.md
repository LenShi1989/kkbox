# kkbox

## KK飛控板 V5.5 採用 Atmel Atmega168晶片

---

| 外部端子 / 模組標示 | GPIO編號 | Pin 腳編號   | 功能與說明                                                          |
| ------------------- | -------- | ------------ | ------------------------------------------------------------------- |
| VCC                 | N/A      | 4,6          | DC 5V                                                               |
| GND                 | N/A      | 3,5          | 接地                                                                |
| RS232-RX            | 18       | 32           | RS232 RX(9600 bps, 8N1, SoftwareSerial)                             |
| RS232-TX            | 17       | 31           | RS232 TX(9600 bps, 8N1, 硬體 UART TXD)                              |
| Roll Gain           | 11       | 26           | 滾轉軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益）                  |
| Pitch Gain          | 12       | 27           | 俯仰軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益）                  |
| Yaw Gain            | 13       | 28           | 偏航軸陀螺儀類比訊號輸入（經可變電阻調整增益）                      |
| Yaw 陀螺儀          | 8        | 23           | Gyro Yaw                                                            |
| Pitch 陀螺儀        | 9        | 24           | Gyro pitch                                                          |
| Roll 陀螺儀         | 10       | 25           | Gyro roll                                                           |
| Motor 1 (M1)        | 2        | 14           | 第 1 號馬達 / ESC 輸出（Quad-X：左前 FL，CCW）                      |
| Motor 2 (M2)        | 1        | 13           | 第 2 號馬達 / ESC 輸出（Quad-X：右前 FR，CW）                       |
| Motor 3 (M3)        | 0        | 12           | 第 3 號馬達 / ESC 輸出（Quad-X：右後 RR，CCW）                      |
| Motor 4 (M4)        | 23       | 11           | 第 4 號馬達 / ESC 輸出（Quad-X：左後 RL，CW）                       |
| Motor 5 (M5)        | 22       | 10           | 第 5 號馬達 / ESC 輸出（X8：左前 FL 下槳，CW）                      |
| Motor 6 (M6)        | 21       | 9            | 第 6 號馬達 / ESC 輸出（X8：右前 FR 下槳，CCW）                     |
| Motor 7 (M7)        | 19       | 1            | 第 7 號馬達 / ESC 輸出（X8：右後 RR 下槳，CW）                      |
| Motor 8 (M8)        | 20       | 2            | 第 8 號馬達 / ESC 輸出（X8：左後 RL 下槳，CCW；D4/PD4）             |
| LED1                | 6        | 7 板載LED1燈 |
| ISP 燒錄器接腳(1)   | 4        | 16           | MISO                                                                |
| ISP 燒錄器接腳(2)   | N/A      | 4,6          | VCC 5V                                                              |
| ISP 燒錄器接腳(3)   | 5        | 17           | SCK                                                                 |
| ISP 燒錄器接腳(4)   | 3        | 15           | MOSI                                                                |
| ISP 燒錄器接腳(5)   | 14       | 29           | RESET                                                               |
| ISP 燒錄器接腳(6)   | N/A      | 3,5          | 接地                                                                |

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
├── kkbox/                        Arduino 韌體 (IDE 開啟 kkbox.ino)
│   ├── kkbox.ino                 主程式: 控制回路 + 序列 JSON 協定
│   ├── config.h                  腳位對照與可調參數
│   ├── pid.h                     PID 演算法
│   ├── gyro.h                    三軸陀螺儀讀取 / 校正 / 卡爾曼濾波
│   ├── mixer.h                   四軸 / 八軸 (X8 同軸) 混控
│   └── jsonutil.h                輕量 JSON 解析 (無動態記憶體)
├── WT32_ETH01_receiver/          WT32-ETH01 接收機 (有線 RJ45 + WiFi 網頁)
│   ├── WT32_ETH01_receiver.ino   主程式: 網路 + 飛控 UART 橋接
│   ├── config.h                  腳位 / 網路 / 預設參數
│   ├── settings.h                NVS 設定儲存 (帳密/WiFi/IP/Token)
│   ├── kkbox.h                   飛控板 UART 橋接
│   ├── web.h                     網頁/WebSocket/REST/OTA/登入
│   ├── build_opt.h               ElegantOTA 非同步旗標
│   └── data/                     SPIFFS 網頁 (index.html/style.css/app.js)
├── ESP32_D1_mini_rc2/            ESP32 D1 mini 遙控器 rc2 (純 WiFi + 硬體/軟體搖桿)
│   ├── ESP32_D1_mini_rc2.ino     主程式 (開機 ID 選擇 / 搖桿校正)
│   ├── config.h                  腳位 / WiFi / 搖桿 / OLED 頁面
│   ├── settings.h                SPIFFS config.json 設定儲存
│   ├── input.h                   硬體搖桿 / 開關 / VRC1 / 軟體搖桿
│   ├── telem.h                   遙測狀態
│   ├── display.h                 SSD1306 OLED 四頁顯示
│   ├── link.h                    WebSocket 用戶端 (連接收機)
│   ├── web.h                     網頁/WebSocket/REST/OTA/登入
│   ├── build_opt.h               ElegantOTA 非同步旗標
│   └── data/                     SPIFFS 網頁 + config.json
└── tools/
    └── kkbox_monitor.py          電腦端序列埠遠端測試工具 (pyserial)
```

## 開發環境

- Arduino IDE, 開啟 `kkbox/kkbox.ino`
- 開發板: `Arduino Pro or Pro Mini` → `ATmega168, 8MHz` (本板依 docs 使用內建 8MHz RC 振盪器)
- 燒錄: 透過 ISP (AVRISP/Arduino as ISP)，或先燒 bootloader 後用 USB-TTL 接硬體 UART (D0 RXD / D1 TXD) 燒錄；RS232 連接器（D1=TX / D2=RX）僅供韌體通訊

## Arduino 腳位對照

GPIO 編號 → ATmega168 埠 → Arduino 腳位（詳見 `kkbox/config.h`）：

| 功能                | GPIO     | 埠          | Arduino   | 說明                          |
| ------------------- | -------- | ----------- | --------- | ----------------------------- |
| RS232 RX            | 18       | PD2         | D2        | 軟體 UART RX (SoftwareSerial) |
| RS232 TX            | 17       | PD1         | D1        | 硬體 UART TXD                 |
| 陀螺儀 Yaw          | 8        | ADC0        | A0        | 壓電陀螺儀                    |
| 陀螺儀 Pitch        | 9        | ADC1        | A1        | 壓電陀螺儀                    |
| 陀螺儀 Roll         | 10       | ADC2        | A2        | 壓電陀螺儀                    |
| Roll/Pitch/Yaw Gain | 11/12/13 | ADC3/4/5    | A3/A4/A5  | 可變電阻（預設停用）          |
| M1 / M2 / M3        | 2/1/0    | PB2/PB1/PB0 | D10/D9/D8 | ESC 輸出                      |
| M4 / M5 / M6        | 23/22/21 | PD7/PD6/PD5 | D7/D6/D5  | ESC 輸出                      |
| M7                  | 19       | PD3         | D3        | ESC 輸出                      |
| M8                  | 7        | PB7         | (XTAL2)   | 僅內建振盪器時可用            |
| LED1                | 6        | PB6         | (XTAL1)   | 板載 LED，用直接埠控制        |

> 注意：LED1 (PB6) 與 M8 (PB7) 是標準 Arduino 的晶體腳。本板使用內建振盪器才可當 GPIO；
> 若外接晶體，請在 `config.h` 將 `LED1_DIRECT` 設為 0 並改用其他腳位。
>
> RS232 採 **TX=硬體 UART、RX=SoftwareSerial**（`kkbox/config.h`）：
> TX=D1(GPIO17) 走硬體 UART TXD（與 RX 不同腳），RX=D2(GPIO18) 為 SoftwareSerial；
> 硬體 UART 的 D0(RXD) 已釋出並停用（停 RX 中斷 + 內部上拉，免懸空雜訊）。
> 硬體 UART 發送由 UDRE 中斷驅動、**不關中斷**，遙測輸出不會屏蔽命令接收；
> SoftwareSerial 發送則逐字元關中斷（cli），是先前「命令時好時壞／延遲收到」的主因。
> 鮑率 **9600**：飛控板 RX 為 8MHz SoftwareSerial，實測僅 9600 能可靠收到命令；
> 設 57600 時位元時間僅 ~104µs（8MHz 下約 139 週期）取樣跟不上 → 飛控收不到命令。
> TX 已改硬體 UART（UDRE 中斷驅動、發送不關中斷），低速不再妨礙命令接收；
> 遙測週期設 200ms（9600 下一幀約 160 位元組 ≈ 167ms），避免 TX 緩衝滿造成阻塞。
> ISP 燒錄（PROISP）走 SPI 與 UART 無關，不受影響。

## 序列埠 JSON 協定

鮑率 **9600 8N1**，每行一筆 JSON，以 `\n` 結尾。

### 電腦 → 飛控板（控制命令）

```json
{"cmd":"ping"}
{"cmd":"status"}
{"cmd":"led","mode":"on"}                                // on / off / blink，或 {"cmd":"led","value":1}
{"cmd":"arm"}                                            // 解鎖後油門歸零, 開始 PID 平衡
{"cmd":"disarm"}                                         // 上鎖, 馬達停止
{"cmd":"calibrate"}                                      // 陀螺儀零點校正 (機體需靜止)
{"cmd":"throttle","value":1300}                          // 油門 1000~2000 us
{"cmd":"setpoints","roll":0,"pitch":0,"yaw":0}           // 角速率目標 (deg/s)
{"cmd":"pid","axis":"roll","kp":3.0,"ki":0.0,"kd":8.0}
{"cmd":"telemetry","value":50}                           // 遙測週期 ms, 0 = 關閉
{"cmd":"motors","count":8}                               // 飛行模式: 馬達數 4 / 8 (8 = X8 同軸)
{"cmd":"hold","on":1}                                    // 定高 (油門保持): 鎖定當前油門, on: 0/1
```

### 飛控板 → 電腦（遙測，含三軸陀螺儀資料）

```json
{
  "t": 12345,
  "armed": 1,
  "mcount": 4,
  "hold": 0,
  "gyro": { "roll": 1.23, "pitch": -4.56, "yaw": 0.78 },
  "set": { "roll": 0.0, "pitch": 0.0, "yaw": 0.0, "thr": 1300 },
  "motor": [1420, 1380, 1460, 1350]
}
```

`mcount` 為目前馬達數 (4 或 8)，`motor` 陣列長度隨之 (8 顆時為 `[M1..M8]`)；`hold` 為定高 (油門保持) 狀態。

回應命令時回傳 `{"ack":"..."}`；解鎖狀態下若 2 秒內未收到任何命令會回報 `{"evt":"failsafe"}` 並自動上鎖。

## 馬達模式 (4 / 8) 與定高

- **馬達模式**：開機預設 4 槳，可用 `{"cmd":"motors","count":8}` 執行中切換為 8 槳 (X8 同軸)。
  - 4 槳：Quad-X（`KK_FRAME=0`，預設，配置見下節）或 Quad-+（`KK_FRAME=1`，編譯期；需照 M1 前 / M2 右 / M3 後 / M4 左 接線）。
  - 8 槳 X8 同軸：4 臂、每臂上下兩顆反槳；上槳 M1~M4 位置/轉向同下節，下槳 M5~M8 依序對應同臂（M5=FL、M6=FR、M7=RR、M8=RL），同 roll/pitch、yaw 反號。
  - M8 於本板改接 **D4（PD4 / GPIO20）**；原 docs 的 M8=GPIO7(PB7/XTAL2) 無法當 GPIO。切換後 Servo 會重新掛載，建議於解鎖前設定。
- **定高（油門保持）**：`{"cmd":"hold","on":1}` 鎖定當下油門，之後的 `throttle` 命令一律忽略，直到 `{"cmd":"hold","on":0}`；`arm`/`disarm` 會自動清除。本板無高度感測器，故屬「定油門」而非真正的定高。
- **自動配平（auto-trim）**：依三軸陀螺儀狀態自動修正平衡。roll/pitch 的積分項（`PID_ROLL_KI`/`PID_PITCH_KI`，預設 `0.08`）**只在搖桿置中時累積**，緩慢學習配平量以抵銷機體/ESC 的固定偏壓，讓機體不再持續漂移；搖桿離開中央即凍結積分、保留已學習的配平值。積分上限為 `PID_I_LIMIT`（`50`）。置中判定範圍為 `TRIM_DEADBAND`（`10` deg/s）。欲關閉請把 roll/pitch `KI` 設為 `0`。
- **資源**：kkbox 韌體 12620 B（88% Flash）、RAM 833 B（81%）——ATmega168 仍吃緊，新增功能務必留意空間。
  - 卡爾曼濾波約增加 0.4 KB Flash；`jsonutil.h` 已改為手寫鍵值比對與整數解析，避開 `snprintf`/`strtol`（會連帶拉進 `vfprintf`、`strtol` 約 2.1 KB），才騰出空間。

### Quad-X 馬達配置（`KK_FRAME=0`，預設）

```
          機頭 ↑
    M1(FL) CCW     M2(FR) CW
            \   /
             \ /
              X
             / \
            /   \
    M4(RL) CW      M3(RR) CCW
          機尾 ↓
```

| 馬達 | 位置 | 轉向 | 對角同向 | X8 下槳 |
| --- | --- | --- | --- | --- |
| M1 | 左前 FL | 逆時針 CCW | M3 | M5（CW） |
| M2 | 右前 FR | 順時針 CW | M4 | M6（CCW） |
| M3 | 右後 RR | 逆時針 CCW | M1 | M7（CW） |
| M4 | 左後 RL | 順時針 CW | M2 | M8（CCW） |

- 對角馬達同轉向（M1/M3 = CCW、M2/M4 = CW），每顆馬達裝對應轉向的槳；位置或轉向錯誤會直接翻機。
- 遙測 `motor` 陣列順序即 M1→M8，可直接對照上表檢查各馬達輸出。
- X8 同軸時下槳（M5~M8）與同臂上槳轉向相反，故 yaw 修正反號，roll/pitch 修正方向相同。
- 舊版註解標示（M1=FR、M2=RR、M3=RL、M4=FL）與混控係數實際對應不符，本版已統一為 **M1=FL、M2=FR、M3=RR、M4=RL**；升級後請依上表重新核對馬達接線。

## 板載 LED1 狀態燈

kkbox 飛控板上的 **LED1 狀態燈**（PB6 / Arduino pin 13）用來即時顯示飛控狀態，並可由遠端切換模式：

| 模式            | 命令                           | 顯示                                                                           |
| --------------- | ------------------------------ | ------------------------------------------------------------------------------ |
| `blink`（預設） | `{"cmd":"led","mode":"blink"}` | **未解鎖**：慢閃（約 2Hz，500ms 週期）；**已解鎖**：快閃（約 20Hz，25ms 週期） |
| `on`            | `{"cmd":"led","mode":"on"}`    | 恆亮                                                                           |
| `off`           | `{"cmd":"led","mode":"off"}`   | 恆暗                                                                           |

- 開機預設為 `blink`：可由閃爍速度一眼判斷是否已解鎖（快閃＝已解鎖，慢閃＝上鎖）。
- LED 模式與狀態可由接收機或遙控器網頁的指令轉送設定，指令等同 `{"cmd":"led","mode":"on"}`（或 `{"cmd":"led","value":1}`）。

### 遠端控制板載 LED1 測試

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

## 無線傳輸架構（WT32-ETH01 接收機 + ESP32 D1 mini 遙控器）

```
KK飛控板 ──序列埠(9600 8N1)── WT32-ETH01 接收機 ──RJ45/WiFi── 路由器 ──WiFi── ESP32 D1 mini 遙控器
   (Atmega168)            (有線+無線 網頁/WS)                          (OLED + 網頁模擬搖桿)
                                 │
                                 └── 瀏覽器 (有線或無線網頁，即時 WebSocket)
```

- 兩台裝置皆為 **Arduino(.ino)** 專案，各自提供 **SPIFFS 網頁介面**（側邊欄）與 **WebSocket 即時雙向**。
- 網頁需登入（**登入頁 + Session Token**，預設帳號 `admin` / 密碼 `kkbox`）。
- 協定沿用飛控板的 JSON 命令；接收機只做轉送，遙控器端再於網頁喬狀控制。

### 資料傳輸路徑（控制命令: 遙控器 → 接收機 → 飛控板）

```
ESP32 D1 mini 遙控器 (網頁喬狀 / 模擬搖桿)
  │ ① WiFi WebSocket /ws  (或 REST POST /api/command)
  │    linkSend() 以 WebSocket 用戶端送出
  ▼
WT32-ETH01 接收機
   │ ② onWsEvent() 收到 { ... } → kkSendLog() → kkSend()
   │    Serial2 RS232 (9600 8N1, GPIO5=RX / GPIO17=TX)，自動補 '\n'
  ▼
kkbox 飛控板 (ATmega168)
     readSerial() 逐行讀取 → handleCommand() 解析 JSON 執行
```

- 命令字串**原樣轉送**（接收機不修改內容）；轉送同時以 `{"ser":{"dir":"tx","data":"..."}}` 廣播，供「RS232 串口通訊」頁即時顯示。
- 回程（遙測 / ack / evt）方向相反：飛控板 `rs232` → 接收機 `kkReadLine()` → WebSocket **原樣廣播** → 遙控器 / 瀏覽器。
- 對應程式碼：`ESP32_D1_mini_rc2/link.h`（`linkSend`）、`WT32_ETH01_receiver/web.h`（`kkSendLog`）、`WT32_ETH01_receiver/kkbox.h`（`kkSend`）、`kkbox/kkbox.ino`（`readSerial` / `handleCommand`）。

### 共用程式庫

| 程式庫                                           | 用途                              |
| ------------------------------------------------ | --------------------------------- |
| `ESP Async WebServer`（ESP32Async）+ `Async TCP` | 非同步網頁伺服器與 WebSocket      |
| `ElegantOTA`                                     | 網頁 OTA 韌體更新                 |
| `WebSockets`（arduinoWebSockets）                | 遙控器以 WebSocket 用戶端連接收機 |
| `Adafruit SSD1306` + `Adafruit GFX Library`      | 遙控器 OLED                       |
| `ArduinoJson`（v7）                              | 遙控器解析遙測                    |
| `NimBLE-Arduino`（1.4.x）                        | 接收機/遙控器 BLE 連線            |

> ElegantOTA 需在 `build_opt.h` 定義 `-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`（兩專案已附）。
> 兩專案的網頁檔在 `data/` 目錄，需上傳至裝置的 SPIFFS（見下方「燒錄方法 SOP」）。

### WT32-ETH01 接收機（`WT32_ETH01_receiver/`）

- 開發板：`esp32:esp32:wt32-eth01`（Arduino IDE：`WT32-ETH01 Ethernet Module`）
- **分區請選 `Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)`**（`PartitionScheme=min_spiffs`；程式約 1.1MB / 55%，保留 128KB SPIFFS 供網頁與設定）。
- 功能：RJ45（LAN8720A）+ WiFi、飛控板 UART 橋接、SPIFFS 網頁、WebSocket、OTA、登入。**2.1.0 起移除全部藍牙功能**（`bt.h` / `spp.h` 已刪除，遙控器一律走 WiFi WebSocket）。
- 網頁側邊欄：**系統狀態**（WiFi/RJ45/記憶體/SPIFFS/即時遙測）、**網路設定**（WiFi 掃描/儲存/清除、RJ45 DHCP/固定 IP）、**KKBOX 飛控管理**（**馬達模式切換**：4/8 顆馬達、**飛行狀態**：無人機 ID（接收機自身設定）+ 油門 PWM 儀表盤 + 馬達 PWM 圓餅圖、**RS232 串口通訊**：即時顯示 TX/RX JSON 字串、統計筆數、暫停/清除、手動送命令）、**OTA 更新**、**使用者設定**（帳密 + 裝置連結 Token + 本機無人機 ID）。

> PID 設定頁已**移至遙控器**（見下方 ESP32 D1 mini 遙控器 rc2）。

> 舊版（2.0.x）的 BLE / 傳統藍牙 SPP Server 已於 2.1.0 移除；接收機不再廣播 `KKBOX-RX-*`，也不再提供 `/api/bt`。遙測與命令只經 WiFi WebSocket（`/ws`）或 RJ45 上的同一個 WebSocket。

| WT32-ETH01  | 連接對象          | 說明                            |
| ----------- | ----------------- | ------------------------------- |
| GPIO5 (RX)  | KKBOX RS232-TX 17 | 5V→3.3V 位準轉換                |
| GPIO17 (TX) | KKBOX RS232-RX 18 | 3.3V→5V 位準轉換                |
| GND         | KKBOX GND         | 務必共地                        |
| RJ45        | 路由器 / 交換器   | 有線網路（可用 DHCP 或固定 IP） |

> RJ45（LAN8720A）使用 GPIO16(RST)/18(MDIO)/23(MDC) 與 GPIO0 時脈輸入，已於 `config.h` 設定。

### ESP32 D1 mini 遙控器 rc2（`ESP32_D1_mini_rc2/`）

- 開發板：`esp32:esp32:d1_mini32`（Arduino IDE：`WEMOS D1 MINI ESP32`），分區選 `Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)`。
- **只用 2.4G WiFi**：以 WebSocket 用戶端連線 WT32-ETH01 接收機；已移除藍牙功能。
- **硬體遙控**：四軸搖桿 (ADC1) 20Hz 連續送出 `setpoints` / `throttle`；SW3 = ARM、SW4 = DISARM。
- **軟體遙控**：網頁模擬搖桿；控制來源可選「自動 / 硬體 / 軟體」。自動模式下使用軟體搖桿後會持續由軟體控制，直到實體搖桿實際移動才交還硬體。
- **VRC1 切換 OLED 頁面**：0 = 飛行遙測、1 = 硬體輸入、2 = 網路資訊、3 = ID / 校正值。版面為「標題列 + 分隔線 + 5 行內容（y=12/22/32/42/52）」，最底行結束於 y60，不會再被裁切；畫面文字一律 ASCII（Adafruit 內建字型不支援中文）。
- **SPIFFS `/config.json`**：帳密、WiFi、接收機 IP/Token、無人機 ID、控制來源、搖桿校正、PID 全部存在同一個 JSON。
- 開機流程：轉 VRC1 選無人機 ID → 按 SW1 確認 → **按住 SW1 期間把四軸推到兩端校正** → 放開 SW1 結束；**按住 SW2 開機可跳過**（沿用已存的 ID / 校正值，尚未校正過則仍會進入流程）。
- 網頁側邊欄：系統狀態、網路設定、PID 設定、模擬搖桿、**硬體遙控**（即時 ADC / 開關 / VRC1 / 校正 / 控制來源）、馬達校正、OTA、使用者設定。

| ESP32 D1 mini               | 連接對象          | 說明                               |
| --------------------------- | ----------------- | ---------------------------------- |
| GPIO39/35/33/34             | VTX/VTY/VPIT/VROL | 四軸搖桿 ADC1                      |
| **GPIO36 (VP)**             | VRC1              | **改接 ADC1**（原 GPIO13 為 ADC2） |
| GPIO25/32/12                | VRC2/3/4          | 保留定義；25/12 為 ADC2，WiFi 下不可用 |
| GPIO10/18/19/23             | SW1~4             | 開關（INPUT_PULLUP，LOW=ON）       |
| GPIO17 (SDA) / GPIO16 (SCL) | SSD1306           | I2C                                |
| GPIO2                       | 板上 LED          | 連線指示（未連線閃較快）           |

> 重要：ESP32 開啟 WiFi 後 **ADC2 無法讀取**，所以 VRC1 必須從 GPIO13 改接 **GPIO36**；其他 ADC2 腳位（GPIO25/26/27/12~15、GPIO0/2/4）也不要用於搖桿。
> 注意：SW1 = GPIO10 在多數 ESP32-WROOM 模組屬 Flash 腳；若開機異常請改 `config.h` 的 `PIN_SW1` 並同步接線。

### 首次使用流程

1. 接收機接上 RJ45（開機後一律啟動 SoftAP `KKBOX-Receiver-<MAC後4碼>` / `12345678`，STA 與 AP 共存），瀏覽器開啟其 IP，以 `admin`/`admin` 登入；可於「使用者設定」設定本機無人機 ID（顯示於飛行狀態頁）。
2. 於「使用者設定」複製 **裝置連結 Token**。
3. 遙控器開機（一律啟動 SoftAP `KKBOX-RC-<MAC後4碼>` / `12345678`，STA 與 AP 共存），登入後於「網路設定」填入 **接收機 IP 與 Token**，即自動建立 WebSocket 連線。
4. 進入「模擬搖桿」或「硬體遙控」即可解鎖、控制油門與 Roll/Pitch/Yaw；硬體搖桿第一次使用請先完成開機校正。

### WebSocket 協定

- 接收機：`/ws`（瀏覽器與遙控器皆連此）。連線可帶 `?token=<Session或裝置Token>`，或使用登入 cookie。
- 接收機把飛控板每行 JSON **原樣廣播**給所有用戶端；用戶端送出的 JSON 會**原樣轉送**飛控板。
- 接收機轉送命令時另廣播監控幀 `{"ser":{"dir":"tx","data":"<命令字串>"}}`；「RS232 串口通訊」頁以此顯示 TX，並把收到的原始行顯示為 RX。
- 遙控器：`/ws` 供其網頁喬狀使用；遙控器同時是接收機的 WebSocket 用戶端，會把遙測轉發給本地網頁、把喬狀命令轉送接收機。

控制命令（飛控板 JSON 協定，經 WebSocket/HTTP 傳送）：

```json
{"cmd":"arm"}   {"cmd":"disarm"}   {"cmd":"calibrate"}
{"cmd":"throttle","value":1300}
{"cmd":"setpoints","roll":0,"pitch":0,"yaw":0}
{"cmd":"led","mode":"on"}
```

## 燒錄方法 SOP（WT32_ETH01_receiver / ESP32_D1_mini_rc2）

> 每個專案都要燒 **程式（`.ino`）** 與 **網頁（`data/` → SPIFFS）** 兩部分，缺一網頁會 404。
> 專案內的 `build_opt.h`（`-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`）是編譯必要檔，請勿刪除。
> 各專案彼此獨立，直接開啟該資料夾內的 `.ino` 即可（IDE 會自動載入同資料夾的 `config.h/web.h/...`）。

### 0. 前置準備

1. **Arduino IDE 2.x**（或 `arduino-cli`）。
2. 安裝 ESP32 開發板：`檔案 → 偏好設定 → 額外開發板管理員網址` 填入
   `https://espressif-download.espressif.com/dl/package_esp32_index.json`，
   再於 `開發板管理員` 搜尋安裝 **esp32 by Espressif**（本專案以 **3.3.x** 驗證）。
3. 安裝程式庫（程式庫管理員）：
   `ESP Async WebServer`、`Async TCP`、`ElegantOTA`、`WebSockets`、
   `Adafruit SSD1306`、`Adafruit GFX Library`、`ArduinoJson`（v7）、`NimBLE-Arduino`（1.4.x）。
4. 驅動：USB-TTL（CH340/CP2102/FTDI）與遙控器的 USB 晶片驅動。

### 1. 接收機（`WT32_ETH01_receiver/`）

WT32-ETH01 沒有 USB，必須用 **USB-TTL（3.3V，勿用 5V TTL）** 燒錄。

| USB-TTL | WT32-ETH01 | 說明                   |
| ------- | ---------- | ---------------------- |
| TX      | RXD        | 交叉                   |
| RX      | TXD        | 交叉                   |
| GND     | GND        | 務必共地               |
| 3V3     | 3V3        | 供電（或板子另接電源） |

步驟：

1. 將 **IO0 接 GND**（進入下載模式）。
2. 接上 USB-TTL，Arduino IDE 開發板選 **`WT32-ETH01 Ethernet Module`**（FQBN `esp32:esp32:wt32-eth01`），
   **Partition Scheme 選 `Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)`**（程式約 1.35MB，用預設分區會編譯失敗）。
3. 選對序列埠 → 按 **上傳**（編譯並燒錄程式）。
4. 燒錄完成後，**移開 IO0 的接地** 並按 RESET（或重新上電）進入執行。

### 2. 遙控器（`ESP32_D1_mini_rc2/`）

1. 以 micro-USB 連接電腦。
2. 開發板選 **`WEMOS D1 MINI ESP32`**（FQBN `esp32:esp32:d1_mini32`）。`rc2` 約 1.30MB（66%，min_spiffs），使用其他分區會與 SPIFFS 位移不符。
3. **Partition Scheme 選 `Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)`**
   ——程式約 1.30MB，用預設分區（1.25MB）會編譯失敗。
4. 按 **上傳**。若出現連線失敗，按住板上的 **BOOT(IO0)** 鍵再上傳。

### 3. 燒錄網頁檔（`data/` → SPIFFS）

`.ino` 不含網頁，`data/` 內容必須另外寫入 SPIFFS：

- **專案附的打包腳本（推薦）**：會自動尋找 mkspiffs 並依專案判斷分割大小，輸出至各專案的 `build\spiffs.bin`。
  ```powershell
  tools\build_spiffs.bat WT32_ETH01_receiver   # size 0x20000 (offset 0x3D0000)
  tools\build_spiffs.bat ESP32_D1_mini_rc2     # 內含 data\config.json
  ```
  （兩個專案皆使用 `Minimal SPIFFS` 分割表，SPIFFS 位移同為 `0x3D0000`、大小 `0x20000`。`rc2` 首次開機若找不到 `/config.json` 會自動以預設值建立。）
  產生後再用 esptool 燒入：`esptool.py --chip esp32 --port COMx write_flash <offset> build\spiffs.bin`。
- **Arduino IDE 1.8.x**：安裝 **ESP32 Sketch Data Upload** 外掛，
  使用 `Tools → ESP32 Sketch Data Upload`（分割區會依所選開發板自動帶入）。
- **Arduino IDE 2.x**：官方尚未支援外掛，請改用下列任一方式：
  - **PlatformIO**：開啟專案後執行 `pio run -t uploadfs`（會依 partition table 自動燒入）。
  - **arduino-cli + mkspiffs/esptool**：
    1. 由 ESP32 核心的 `tools/partitions/<分割表>.csv` 查出 `spiffs` 分區的 **offset** 與 **size**；
    2. 打包：`mkspiffs -c data -b 4096 -p 256 -s <size> spiffs.bin`；
    3. 燒錄：`esptool.py --chip esp32 --port COMx write_flash <offset> spiffs.bin`。

### 4. 開機與驗證

- 序列埠監控 **115200 8N1** 可看開機 log（會顯示 WiFi/IP/SPIFFS 狀態）。
- **接收機**：接 RJ45 會以 DHCP 取得 IP；同時一律啟動 SoftAP
  `KKBOX-Receiver-<MAC後4碼>` / 密碼 `12345678`（WiFi STA 連線與 AP 共存，AP 網段 `192.168.4.x`）。
  瀏覽器開啟其 IP，登入 `admin / admin`。
- **遙控器**：一律啟動 SoftAP `KKBOX-RC-<MAC後4碼>` / 密碼 `12345678`（WiFi STA 連線與 AP 共存，AP 網段 `192.168.10.x`，與接收機區隔），並於 OLED 顯示狀態。
- 首次設定：接收機「使用者設定」取得 **裝置連結 Token**；遙控器「網路設定」填入接收機 **IP + Token**。
- **rc2 開機流程**：轉 VRC1 選無人機 ID → 按 SW1 確認 → 按住 SW1 期間把四軸推到兩端校正 → 放開 SW1 結束；按住 SW2 開機則跳過（沿用 `/config.json` 內的 ID/校正值）。硬體搖桿未完成校正前不會送出命令。
- **遙控器連線**：接收機（2.1.0+）已無藍牙，遙控器請用 `ESP32_D1_mini_rc2` 並在「網路設定」填入接收機 IP + Token，狀態頁的 `LINK` 亮起即代表 WebSocket 已連線。

### 5. 之後的更新：OTA

- 瀏覽器開啟 `http://<裝置IP>/update`，直接上傳對應專案的 `.bin`（**無需帳密驗證**）
  （Arduino IDE：`草稿碼 → 匯出已編譯二進位檔`）。
- OTA **只更新程式**，不會更新 SPIFFS 網頁；改網頁仍需回到步驟 3。

### 主要 REST API（兩裝置共通，除註明外）

| 方法     | 路徑                    | 說明                                                                                                                                                                                                          |
| -------- | ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| POST     | `/api/login`            | `user`,`pass` → `{token}`；設 Cookie `sess`                                                                                                                                                                   |
| POST     | `/api/logout`           | 登出                                                                                                                                                                                                          |
| GET      | `/api/status`           | 系統/網路/連線狀態                                                                                                                                                                                            |
| GET      | `/api/telemetry`        | 最新飛控遙測                                                                                                                                                                                                  |
| POST     | `/api/command`          | `cmd`＝JSON 命令字串，轉送飛控板                                                                                                                                                                              |
| GET      | `/api/fs`               | SPIFFS 容量與檔案清單                                                                                                                                                                                         |
| GET      | `/api/scan`             | WiFi 掃描                                                                                                                                                                                                     |
| POST     | `/api/wifi`             | `ssid`,`pass` 或 `action=clear`                                                                                                                                                                               |
| GET/POST | `/api/user`             | 帳號設定（POST 需 `cur` 目前密碼）                                                                                                                                                                            |
| GET/POST | `/api/net`（僅接收機）  | RJ45 DHCP/固定 IP（儲存後重開機）                                                                                                                                                                             |
| GET/POST | `/api/pid`（僅遙控器）  | 三軸 PID（rc 存 NVS、rc2 存 `config.json`，經 WiFi 下傳飛控板）                                                                                                                                                                  |
| GET/POST | `/api/link`（僅遙控器） | 接收機 IP/埠/Token                                                                                                                                                                                            |
| GET      | `/api/hardware`（僅 rc2）| 即時 ADC / 開關 / VRC1 / 輸出 / 校正值 / 控制來源 |
| POST     | `/api/cal`（僅 rc2）    | `action=start\|stop` 搖桿校正 |
| POST     | `/api/control`（僅 rc2）| `source`＝0 自動／1 硬體／2 軟體 |
| GET/POST | `/api/id`（rc2 / 接收機）| rc2：`drone`,`rc`；接收機：`id`（本機無人機 ID） |
| GET      | `/api/config`（僅 rc2） | 回傳 SPIFFS `/config.json` 原始內容 |
| —        | `/update`               | ElegantOTA 上傳頁（HTTP 驗證）                                                                                                                                                                                |

> 所有 `/api/*`（除 `login`/`logout`）需帶 Session Cookie 或 `X-Auth` Token，否則回 `401`。

## PID 調校

本板沒有加速度計，只有三軸壓電陀螺儀，因此是 **rate PID（角速度回授）**，也就是 KK 板的「自主平衡」阻尼：搖桿給的是**目標角速度**（deg/s），PID 讓實際角速度追上它。沒有自平功能，起飛仍需手動修正。

### 參數總覽

| 參數 | 預設 | 位置 | 作用 |
| --- | --- | --- | --- |
| `PID_ROLL_KP` / `PID_PITCH_KP` | 3.0 | `config.h` | 主要回正力道；太小軟、太大高頻抖動 |
| `PID_ROLL_KD` / `PID_PITCH_KD` | 8.0 | `config.h` | 阻尼，抑制震盪；太大馬達發熱、槳聲變尖 |
| `PID_ROLL_KI` / `PID_PITCH_KI` | 0.08 | `config.h` | 自動配平（僅搖桿置中時累積） |
| `PID_YAW_KP` / `KI` / `KD` | 4.0 / 0.5 / 0.0 | `config.h` | 鎖定尾巴；KI 消除緩慢自轉 |
| `PID_OUT_LIMIT` | 100 us | `config.h` | 單軸修正量上限 |
| `PID_I_LIMIT` | 50 | `config.h` | 積分上限（抗積分飽和） |
| `CTRL_HZ` | 100 Hz | `config.h` | 陀螺取樣 / PID / 馬達輸出頻率 |
| `GYRO_DEG_PER_COUNT` | 0.6 | `config.h` | ADC count → deg/s，決定 PID 增益尺度 |
| `GYRO_KALMAN_*` | 見下節 | `config.h` | 陀螺儀濾波 |

三軸 PID 也可在**遙控器網頁「PID 設定」**即時調整（存本機 NVS，經 WiFi/藍牙下傳），或直接送 JSON：

```json
{"cmd":"pid","axis":"roll","kp":3.0,"ki":0.08,"kd":8.0}
```

> 網頁調的是飛控板 RAM 內的即時值，**重開機回到 `config.h` 預設**；調到滿意請把數值寫回 `config.h`。

### 調校前檢查（務必先做）

1. **拆槳**或把機體牢牢固定（拆槳最安全），確認馬達序號、轉向與接腳正確。
2. 送 `{"cmd":"calibrate"}` 做陀螺零點校正（`GYRO_CAL_SAMPLES=400`，約 0.8 秒，機體必須完全靜止）。
3. 開遙測觀察 `gyro`：靜止時三軸應在 0 附近小幅跳動（約 ±1~2 deg/s），數值會隨卡爾曼濾波收斂。
4. 手動轉動機體，確認三軸對應正確（正負取決於陀螺儀安裝方向，重點是「哪一軸在動」）：
   - 向右滾（右翼下沉）→ 只有 roll 明顯變化。
   - 機頭抬起 → 只有 pitch 明顯變化。
   - 機頭左右轉 → 只有 yaw 明顯變化。
   若某一軸的動作跑到別軸上，代表接腳或設定對應錯誤，先修正。
5. **拆槳**後解鎖，油門加到略低於起飛轉速，用手把機體往右壓（右側下沉）：PID 應讓**右側馬達加速**把機體推回來。若反而加速左側，把 `GYRO_REVERSE_ROLL` 設為 1；機頭下壓時應是後方馬達加速（否則改 `GYRO_REVERSE_PITCH`）；機頭被推向左時馬達應把機頭推回右（否則改 `GYRO_REVERSE_YAW`）。方向錯誤會直接翻機，確認前**不要起飛**。

### 調校順序

**0. 先確認濾波器**（見下一節）：預設卡爾曼 `Q=2.0` / `R=2.0` 已適合一般機架，機體振動大再調整。

**1. Kp（先只調 P：`KI=0`、`KD=0`）**

- 從 `kp = 1.0` 開始，每次加 0.5~1.0（roll/pitch 一起調，yaw 先不動）。
- 加到開始**快速來回震盪**為止，記下當時數值 `Kp_osc`。
- 退回 `Kp ≈ 0.5 ~ 0.7 × Kp_osc`；預設 `3.0` 是多數 250 級機架的安全起點。
- 太小：機體軟、風一吹就歪、修正慢；太大：槳聲變尖、機體高頻抖動、馬達發燙。

**2. Kd（阻尼）**

- Kp 固定後，`kd` 從 `0` 每次加 2，直到高頻抖動消失、停止動作時不來回擺。
- 出現**馬達發熱、槳聲尖銳、更高頻的嘰嘰聲**就是 Kd 過大（微分放大噪聲）。
- roll/pitch 通常落在 Kp 的 2~3 倍（預設 3.0 / 8.0 約 2.7 倍）；yaw 一般 `kd = 0`。

**3. Ki**

- roll/pitch 的 Ki 在本韌體是**自動配平（auto-trim）**：只在搖桿命令接近 0（`TRIM_DEADBAND = 10 deg/s`）時累積，用來抵銷重心偏移與 ESC 偏壓。0.05~0.15 足夠，預設 0.08。
- 太大 → 機體低頻左右緩慢擺動（hunting）；太小 → 放手後仍持續慢慢偏。
- yaw 的 Ki（預設 0.5）作用不同：消除持續自轉，太大同樣會低頻擺頭。
- 想完全關閉自動配平：`PID_ROLL_KI = PID_PITCH_KI = 0`。

**4. 限幅**

- `PID_OUT_LIMIT = 100 us` 限制單軸修正量；太小修正不足（大動作救不回），太大馬達轉速忽大忽小、電流與聲音都很兇。
- 若 Kp 已加到極限仍覺得「救不回來」，先確認 `GYRO_DEG_PER_COUNT` 是否正確，再考慮放寬到 150~200 us。

**5. `GYRO_DEG_PER_COUNT`**

- 決定 ADC 讀值換算成 deg/s 的比例，等於決定 PID 增益的尺度。換了陀螺儀模組或轉動增益可變電阻後必須重新標定：實際轉 90 deg/s，比對遙測 `gyro` 讀值來調整。

### 症狀對照表

| 症狀 | 可能原因 | 處理 |
| --- | --- | --- |
| 一離地就翻 | 陀螺方向反、馬達序號/轉向錯、PID 抵抗方向錯 | 重新檢查 `GYRO_REVERSE_*` 與馬達配置，拆槳測試 |
| 高頻左右抖動（槳聲尖） | Kp 太大、Kd 太大、濾波不足、槳不平衡 | 降 Kp → 降 Kd → 加強濾波（Q↓ / R↑）→ 平衡槳 |
| 低頻左右緩慢擺動 | Ki 太大（auto-trim 過頭） | 降 roll/pitch Ki（0.03~0.05）或歸零 |
| 放手後一直往同一邊偏 | 重心偏移、auto-trim 尚未學完、馬達/ESC 偏壓 | 調重心；置中停懸 10~20 秒讓 auto-trim 學習 |
| 加油門就抖、收油門就好 | 振動隨轉速上升（槳、馬達軸承、機架共振） | 先做機械平衡，再降 Q／升 R |
| 尾巴慢慢自轉 | yaw Ki 不足或震動干擾 yaw | 微加 `PID_YAW_KI`（0.5→0.8），檢查 yaw 陀螺固定 |
| 打舵反應鈍、風一吹就偏 | Kp 太小或 `PID_OUT_LIMIT` 太小 | 加 Kp，必要時放寬限幅 |
| 馬達燙、耗電快 | Kd 太大、PID 持續高頻修正 | 降 Kd，確認濾波與機械平衡 |

### 驗收標準

- 固定機體、油門略低於起飛點：輕推後能**主動回正且不來回擺**。
- 停懸放手 2~3 秒，機體不持續往同一方向偏（auto-trim 已學到配平）。
- 快速打舵再回中，機體停止乾淨、沒有連續抖動。
- 全程馬達溫度正常、無異常高頻聲。

## 陀螺儀濾波（卡爾曼）

三軸各自一個**一維卡爾曼濾波器**（隨機漫步模型），`GYRO_KALMAN_ENABLE=1`（預設）時取代原本的一階低通 `GYRO_LPF`：

```
預測: x = x,        P = P + Q
更新: K = P/(P+R),  x += K*(z-x),  P = (1-K)*P
```

- 第一筆量值直接初始化濾波器；`calibrate` 會重置濾波器狀態，校正後第一幀不會被舊值拖住。
- **Q 大** → 更信任新量值（反應快、噪聲多）；**R 大** → 更信任模型（平滑、延遲大）。
- 100 Hz 下 `Q=2.0`、`R=2.0` → 穩態增益約 0.62，等效時間常數約 10 ms，與原本 `GYRO_LPF=0.6`（約 11 ms）幾乎相同，因此既有 PID 預設值可直接沿用。

| 參數 | 預設 | 說明 |
| --- | --- | --- |
| `GYRO_KALMAN_ENABLE` | 1 | 1 = 卡爾曼；0 = 改回一階低通 `GYRO_LPF` |
| `GYRO_KALMAN_Q` | 2.0 | 過程噪聲；越大越跟得上快速變化 |
| `GYRO_KALMAN_R` | 2.0 | 量測噪聲；越大越平滑 |
| `GYRO_KALMAN_P_INIT` | 1.0 | 初始估計誤差協方差（第一筆後的起始信任度） |

**怎麼調**：先維持預設，只在「振動明顯」或「反應太鈍」時才動：

- 機體高頻抖動、PID 一直追噪聲 → **Q 調小（0.5~1.0）或 R 調大（3~5）**。
- 打舵拖泥帶水、延遲感明顯 → **Q 調大（3~4）或 R 調小（1.0~1.5）**。
- 三軸共用同一組參數；若只有某一軸震動特別大，先解決機械問題（槳平衡、馬達軸承、陀螺儀減震）。
- 不確定哪個好：`GYRO_KALMAN_ENABLE` 設 0 與 1 各飛一趟比較，同時看馬達溫度與耗電。

---

# USB ISP 燒錄器 Version 2.0

MX-U2BLSP-V5.00

裝置管理員:

USB裝置:
`USB\VID_03EB&PID_C8B4`

燒錄軟體:
`https://github.com/gen-so/PROGISP-V1.72`

## Arduino IDE

- Arduino IDE, 開啟 `kkbox/kkbox.ino`
- 開發板: `Arduino Pro or Pro Mini` → `ATmega168 (3.3V, 8MHz)`

- 匯出已編譯的二進位檔
  - Blink.ino.eightanaloginputs.hex

## PROGISP 燒錄軟體

- 狀態 A：PRG ISP 可用
  - 初步成功
  - PRG ISP 狀態區塊呈現可用狀態，表示軟體可能已找到相容的燒錄器。

- 狀態 B：PRG ISP 灰色或不可用
  - 尚未確認
  - 可能是燒錄器未連接、通訊協定不相容，或軟體沒有正確存取 USB 裝置。

- Load flash
  - 選擇燒錄檔`Blink.ino.eightanaloginputs.hex`

- Auto
  - 開始燒錄

---

# AVRISP mkII 燒錄器

## Zadig 修改驅動軟體

下載網址:

```txt
https://zadig.akeo.ie/
```

處理驅動程式 (如 Atmel Studio 抓不到裝置)：

- 下載並開啟 Zadig 工具。
- 在上方選單點選 Options -> 勾選 List All Devices。
- 從下拉選單中找到 AVRISP mkII（或類似的 Jungo / AVRISP 裝置）。
- 將右側目標驅動程式切換為 libusb-win32 (v1.2.6.0 或相近版本)，點擊 Replace Driver。

## 支援windows10、windows11 燒錄軟體

下載網址:

```txt
https://www.microchip.com/en-us/tools-resources/archives/avr-sam-mcus
```

## 連接與燒錄步驟

- 開啟 Device Programming 介面：
  - 在上方選單點選 Tools -> Device Programming。

- 選擇燒錄工具與晶片：
  - Tool：下拉選擇 AVRISP mkII。
  - Device：下拉選擇 ATmega168（或 ATmega168A / ATmega168P，視實際晶片而定）。
  - Interface：選擇 ISP。
  - 點擊 Apply。

- 檢查連線狀態：
  - 點擊 Read（在 Device signature 旁邊），如果成功讀出晶片簽章（Device Signature）與 Target Voltage（目標電壓，正常應顯示 3.3V 或 5V），代表硬體連接與驅動完全正常。

- 載入與燒錄 HEX 檔案：
  - 左側選單切換至 Memories 頁籤。
  - 在 Flash 欄位中，點擊 ... 按鈕選擇你編譯好的 .hex 檔案（通常位於專案資料夾下的 Debug 或 Release 目錄中）。
  - 勾選 Verify Flash（燒錄後進行校驗，確保資料正確）。
  - 點擊右下角的 Program 按鈕開始燒錄。

- 完成：
  - 燒錄完成後，下方輸 出視窗會顯示 Flash programming successful，此時可以拔除 ISP 排線或讓晶片開始執行程式。

## Arduino IDE 1.8.19 改直接燒錄

## 安裝 MiniCore（不會直接改寫晶片）

官方專案：MiniCore GitHub。MiniCore 支援 ATmega168 的 P、PA 等版本，並相容 Arduino IDE 1.8.x。
GitHub+1

1. 開啟 Arduino IDE 1.8.19。
2. 點選「檔案 → 偏好設定」。
3. 在「其他開發板管理員網址」加入：

```txt
https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
```

4. 點選「工具 → 開發板 → 開發板管理員」。
5. 搜尋 MiniCore，安裝。
6. 安裝完成後重新啟動 Arduino IDE。
   以上僅安裝開發板定義與工具，不會因此自動修改 KK 飛控板的 Flash 或 Fuse。

## KKmulticontroller V5.5 的設定

在 Arduino IDE 中，選擇「工具」選單，依序設定：

| 選項              | 設定                     |
| ----------------- | ------------------------ |
| Board             | MiniCore → ATmega168     |
| Variant／晶片版本 | ATmega168P（若選單提供） |
| Clock             | 必須符合飛控板實際時脈   |
| Programmer        | AVRISP mkII              |
