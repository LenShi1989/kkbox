# kkbox

## KK飛控板 V5.5 採用 Atmel Atmega168晶片

---

| 外部端子 / 模組標示 | GPIO編號 | Pin 腳編號   | 功能與說明                                         |
| ------------------- | -------- | ------------ | -------------------------------------------------- |
| VCC                 | N/A      | 4,6          | DC 5V                                              |
| GND                 | N/A      | 3,5          | 接地                                               |
| RS232-RX            | 16       | 30           | RS232 RX(115200 bps, 8N1)                          |
| RS232-TX            | 17       | 31           | RS232 TX(115200 bps, 8N1)                          |
| Roll Gain           | 11       | 26           | 滾轉軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益） |
| Pitch Gain          | 12       | 27           | 俯仰軸壓電陀螺儀類比訊號輸入（經可變電阻調整增益） |
| Yaw Gain            | 13       | 28           | 偏航軸陀螺儀類比訊號輸入（經可變電阻調整增益）     |
| Yaw 陀螺儀          | 8        | 23           | Gyro Yaw                                           |
| Pitch 陀螺儀        | 9        | 24           | Gyro pitch                                         |
| Roll 陀螺儀         | 10       | 25           | Gyro roll                                          |
| Motor 1 (M1)        | 2        | 14           | 第 1 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 2 (M2)        | 1        | 13           | 第 2 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 3 (M3)        | 0        | 12           | 第 3 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 4 (M4)        | 23       | 11           | 第 4 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 5 (M5)        | 22       | 10           | 第 5 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 6 (M6)        | 21       | 9            | 第 6 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 7 (M7)        | 19       | 1            | 第 7 號馬達 / 電子調速器（ESC）控制輸出            |
| Motor 8 (M8)        | 7        | 8            | 第 8 號馬達 / 電子調速器（ESC）控制輸出            |
| LED1                | 6        | 7 板載LED1燈 |
| ISP 燒錄器接腳(1)   | 4        | 16           | MISO                                               |
| ISP 燒錄器接腳(2)   | N/A      | 4,6          | VCC 5V                                             |
| ISP 燒錄器接腳(3)   | 5        | 17           | SCK                                                |
| ISP 燒錄器接腳(4)   | 3        | 15           | MOSI                                               |
| ISP 燒錄器接腳(5)   | 14       | 29           | RESET                                              |
| ISP 燒錄器接腳(6)   | N/A      | 3,5          | 接地                                               |

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
│   ├── gyro.h                    三軸陀螺儀讀取 / 校正 / 濾波
│   ├── mixer.h                   四軸混控 (自主平衡輸出)
│   └── jsonutil.h                輕量 JSON 解析 (無動態記憶體)
├── WT32_ETH01_receiver/          WT32-ETH01 接收機 (有線 RJ45 + WiFi 網頁)
│   ├── WT32_ETH01_receiver.ino   主程式: 網路 + 飛控 UART 橋接
│   ├── config.h                  腳位 / 網路 / 預設參數
│   ├── settings.h                NVS 設定儲存 (帳密/WiFi/IP/PID/Token)
│   ├── kkbox.h                   飛控板 UART 橋接
│   ├── web.h                     網頁/WebSocket/REST/OTA/登入
│   ├── build_opt.h               ElegantOTA 非同步旗標
│   └── data/                     SPIFFS 網頁 (index.html/style.css/app.js)
├── ESP32_D1_mini_rc/             ESP32 D1 mini 遙控器 (WiFi + OLED + 網頁)
│   ├── ESP32_D1_mini_rc.ino      主程式
│   ├── config.h                  腳位 / 網路 / 預設參數
│   ├── settings.h                NVS 設定儲存
│   ├── telem.h                   遙測狀態
│   ├── display.h                 SSD1306 OLED 顯示
│   ├── link.h                    WebSocket 用戶端 (連接收機)
│   ├── web.h                     網頁/WebSocket/REST/OTA/登入
│   ├── build_opt.h               ElegantOTA 非同步旗標
│   └── data/                     SPIFFS 網頁 (含模擬搖桿)
└── tools/
    └── kkbox_monitor.py          電腦端序列埠遠端測試工具 (pyserial)
```

## 開發環境

- Arduino IDE, 開啟 `kkbox/kkbox.ino`
- 開發板: `Arduino Pro or Pro Mini` → `ATmega168, 8MHz` (本板依 docs 使用內建 8MHz RC 振盪器)
- 燒錄: 透過 ISP (AVRISP/Arduino as ISP)，或先燒 bootloader 後用 USB-TTL 走 RS232

## Arduino 腳位對照

GPIO 編號 → ATmega168 埠 → Arduino 腳位（詳見 `kkbox/config.h`）：

| 功能                | GPIO     | 埠          | Arduino   | 說明                   |
| ------------------- | -------- | ----------- | --------- | ---------------------- |
| RS232 RX            | 16       | PD0         | D0        | 硬體 UART RXD          |
| RS232 TX            | 17       | PD1         | D1        | 硬體 UART TXD          |
| 陀螺儀 Yaw          | 8        | ADC0        | A0        | 壓電陀螺儀             |
| 陀螺儀 Pitch        | 9        | ADC1        | A1        | 壓電陀螺儀             |
| 陀螺儀 Roll         | 10       | ADC2        | A2        | 壓電陀螺儀             |
| Roll/Pitch/Yaw Gain | 11/12/13 | ADC3/4/5    | A3/A4/A5  | 可變電阻（預設停用）   |
| M1 / M2 / M3        | 2/1/0    | PB2/PB1/PB0 | D10/D9/D8 | ESC 輸出               |
| M4 / M5 / M6        | 23/22/21 | PD7/PD6/PD5 | D7/D6/D5  | ESC 輸出               |
| M7                  | 19       | PD3         | D3        | ESC 輸出               |
| M8                  | 7        | PB7         | (XTAL2)   | 僅內建振盪器時可用     |
| LED1                | 6        | PB6         | (XTAL1)   | 板載 LED，用直接埠控制 |

> 注意：LED1 (PB6) 與 M8 (PB7) 是標準 Arduino 的晶體腳。本板使用內建振盪器才可當 GPIO；
> 若外接晶體，請在 `config.h` 將 `LED1_DIRECT` 設為 0 並改用其他腳位。

## 序列埠 JSON 協定

鮑率 **115200 8N1**，每行一筆 JSON，以 `\n` 結尾。

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
```

### 飛控板 → 電腦（遙測，含三軸陀螺儀資料）

```json
{
  "t": 12345,
  "armed": 1,
  "led": 0,
  "gyro": { "roll": 1.23, "pitch": -4.56, "yaw": 0.78 },
  "set": { "roll": 0.0, "pitch": 0.0, "yaw": 0.0, "thr": 1300 },
  "motor": [1420, 1380, 1460, 1350]
}
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

## 無線傳輸架構（WT32-ETH01 接收機 + ESP32 D1 mini 遙控器）

```
KK飛控板 ──UART(115200)── WT32-ETH01 接收機 ──RJ45/WiFi── 路由器 ──WiFi── ESP32 D1 mini 遙控器
   (Atmega168)            (有線+無線 網頁/WS)                          (OLED + 網頁模擬搖桿)
                                 │
                                 └── 瀏覽器 (有線或無線網頁，即時 WebSocket)
```

- 兩台裝置皆為 **Arduino(.ino)** 專案，各自提供 **SPIFFS 網頁介面**（側邊欄）與 **WebSocket 即時雙向**。
- 網頁需登入（**登入頁 + Session Token**，預設帳號 `admin` / 密碼 `kkbox`）。
- 協定沿用飛控板的 JSON 命令；接收機只做轉送，遙控器端再於網頁喬狀控制。

### 共用程式庫

| 程式庫                                           | 用途                              |
| ------------------------------------------------ | --------------------------------- |
| `ESP Async WebServer`（ESP32Async）+ `Async TCP` | 非同步網頁伺服器與 WebSocket      |
| `ElegantOTA`                                     | 網頁 OTA 韌體更新                 |
| `WebSockets`（arduinoWebSockets）                | 遙控器以 WebSocket 用戶端連接收機 |
| `Adafruit SSD1306` + `Adafruit GFX Library`      | 遙控器 OLED                       |
| `ArduinoJson`（v7）                              | 遙控器解析遙測                    |

> ElegantOTA 需在 `build_opt.h` 定義 `-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`（兩專案已附）。
> 兩專案的網頁檔在 `data/` 目錄，需上傳至裝置的 SPIFFS（見下方「燒錄方法 SOP」）。

### WT32-ETH01 接收機（`WT32_ETH01_receiver/`）

- 開發板：`esp32:esp32:wt32-eth01`（Arduino IDE：`WT32-ETH01 Ethernet Module`）
- 功能：RJ45（LAN8720A）+ WiFi、飛控板 UART 橋接、SPIFFS 網頁、WebSocket、OTA、登入。
- 網頁側邊欄：**系統狀態**（WiFi/RJ45/記憶體/SPIFFS/即時遙測）、**網路設定**（WiFi 掃描/儲存/清除、RJ45 DHCP/固定 IP）、**PID 設定**、**OTA 更新**、**使用者設定**（帳密 + 裝置連結 Token）。

| WT32-ETH01  | 連接對象        | 說明                            |
| ----------- | --------------- | ------------------------------- |
| GPIO5 (RX)  | KKBOX RS232-TX  | 5V→3.3V 位準轉換                |
| GPIO17 (TX) | KKBOX RS232-RX  | 3.3V→5V 位準轉換                |
| GND         | KKBOX GND       | 務必共地                        |
| RJ45        | 路由器 / 交換器 | 有線網路（可用 DHCP 或固定 IP） |

> RJ45（LAN8720A）使用 GPIO16(RST)/18(MDIO)/23(MDC) 與 GPIO0 時脈輸入，已於 `config.h` 設定。

### ESP32 D1 mini 遙控器（`ESP32_D1_mini_rc/`）

- 開發板：`esp32:esp32:d1_mini32`（Arduino IDE：`WEMOS D1 MINI ESP32`）
- **分區請選 `Minimal SPIFFS (Large APPS with OTA)`**（程式約 1.26MB，預設分區僅 1.25MB 會爆；`PartitionScheme=min_spiffs`）。
- 功能：WiFi 連線、以 WebSocket 連接收收機、SSD1306 OLED 顯示、SPIFFS 網頁、模擬搖桿、OTA、登入。
- 網頁側邊欄：**系統狀態**、**網路設定**（WiFi + 接收機連線 IP/Token）、**模擬搖桿**、**OTA 更新**、**使用者設定**。

| ESP32 D1 mini | 連接對象          | 說明     |
| ------------- | ----------------- | -------- |
| GPIO21 (SDA)  | SSD1306 SDA       | I2C 資料 |
| GPIO22 (SCL)  | SSD1306 SCL       | I2C 時脈 |
| 3V3 / GND     | SSD1306 VCC / GND | 供電     |

OLED 顯示內容：

```
KKBOX RC        LINK
ARMED          THR 1300
R  12.3        1420
P  -4.5        1380
Y   0.8        1460
               1350
```

- 右上角狀態：`NO LINK` / `LINK`（是否連上接收機）。
- 左側為陀螺三軸角速度與解鎖/油門；右側為四顆馬達輸出。

### 首次使用流程

1. 接收機接上 RJ45（或開機後無網路 20 秒會啟動 SoftAP `KKBOX-Receiver-Setup` / `kkbox1234`），瀏覽器開啟其 IP，以 `admin`/`kkbox` 登入。
2. 於「使用者設定」複製 **裝置連結 Token**。
3. 遙控器開機（無網路 15 秒會啟動 SoftAP `KKBOX-RC-Setup`），登入後於「網路設定」填入 **接收機 IP 與 Token**，即自動建立 WebSocket 連線。
4. 進入「模擬搖桿」即可解鎖、控制油門與 Roll/Pitch/Yaw。

### WebSocket 協定

- 接收機：`/ws`（瀏覽器與遙控器皆連此）。連線可帶 `?token=<Session或裝置Token>`，或使用登入 cookie。
- 接收機把飛控板每行 JSON **原樣廣播**給所有用戶端；用戶端送出的 JSON 會**原樣轉送**飛控板。
- 遙控器：`/ws` 供其網頁喬狀使用；遙控器同時是接收機的 WebSocket 用戶端，會把遙測轉發給本地網頁、把喬狀命令轉送接收機。

控制命令（飛控板 JSON 協定，經 WebSocket/HTTP 傳送）：

```json
{"cmd":"arm"}   {"cmd":"disarm"}   {"cmd":"calibrate"}
{"cmd":"throttle","value":1300}
{"cmd":"setpoints","roll":0,"pitch":0,"yaw":0}
{"cmd":"led","mode":"on"}
```

## 燒錄方法 SOP（WT32_ETH01_receiver / ESP32_D1_mini_rc）

> 兩個專案都要燒 **程式（`.ino`）** 與 **網頁（`data/` → SPIFFS）** 兩部分，缺一網頁會 404。
> 專案內的 `build_opt.h`（`-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`）是編譯必要檔，請勿刪除。
> 兩個專案各自獨立，直接開啟該資料夾內的 `.ino` 即可（IDE 會自動載入同資料夾的 `config.h/web.h/...`）。

### 0. 前置準備

1. **Arduino IDE 2.x**（或 `arduino-cli`）。
2. 安裝 ESP32 開發板：`檔案 → 偏好設定 → 額外開發板管理員網址` 填入
   `https://espressif-download.espressif.com/dl/package_esp32_index.json`，
   再於 `開發板管理員` 搜尋安裝 **esp32 by Espressif**（本專案以 **3.3.x** 驗證）。
3. 安裝程式庫（程式庫管理員）：
   `ESP Async WebServer`、`Async TCP`、`ElegantOTA`、`WebSockets`、
   `Adafruit SSD1306`、`Adafruit GFX Library`、`ArduinoJson`（v7）。
4. 驅動：USB-TTL（CH340/CP2102/FTDI）與遙控器的 USB 晶片驅動。

### 1. 接收機（`WT32_ETH01_receiver/`）

WT32-ETH01 沒有 USB，必須用 **USB-TTL（3.3V，勿用 5V TTL）** 燒錄。

| USB-TTL | WT32-ETH01 | 說明 |
| --- | --- | --- |
| TX | RXD | 交叉 |
| RX | TXD | 交叉 |
| GND | GND | 務必共地 |
| 3V3 | 3V3 | 供電（或板子另接電源） |

步驟：

1. 將 **IO0 接 GND**（進入下載模式）。
2. 接上 USB-TTL，Arduino IDE 開發板選 **`WT32-ETH01 Ethernet Module`**（FQBN `esp32:esp32:wt32-eth01`）。
3. 選對序列埠 → 按 **上傳**（編譯並燒錄程式）。
4. 燒錄完成後，**移開 IO0 的接地** 並按 RESET（或重新上電）進入執行。

### 2. 遙控器（`ESP32_D1_mini_rc/`）

1. 以 micro-USB 連接電腦。
2. 開發板選 **`WEMOS D1 MINI ESP32`**（FQBN `esp32:esp32:d1_mini32`）。
3. **Partition Scheme 選 `Minimal SPIFFS (1.9MB APP with OTA / 190KB SPIFFS)`**
   ——程式約 1.26MB，用預設分區（1.25MB）會編譯失敗。
4. 按 **上傳**。若出現連線失敗，按住板上的 **BOOT(IO0)** 鍵再上傳。

### 3. 燒錄網頁檔（`data/` → SPIFFS）

`.ino` 不含網頁，`data/` 內容必須另外寫入 SPIFFS：

- **專案附的打包腳本（推薦）**：會自動尋找 mkspiffs 並依專案判斷分割大小，於各專案目錄產生 `spiffs.bin`。
  ```powershell
  tools\build_spiffs.bat WT32_ETH01_receiver   # size 0x160000 (offset 0x290000)
  tools\build_spiffs.bat ESP32_D1_mini_rc      # size 0x20000  (offset 0x3D0000)
  ```
  產生後再用 esptool 燒入：`esptool.py --chip esp32 --port COMx write_flash <offset> spiffs.bin`。
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
- **接收機**：接 RJ45 會以 DHCP 取得 IP；若 20 秒內沒有任何網路，會啟動 SoftAP
  `KKBOX-Receiver-Setup` / 密碼 `kkbox1234`。瀏覽器開啟其 IP，登入 `admin / kkbox`。
- **遙控器**：無網路 15 秒後啟動 SoftAP `KKBOX-RC-Setup` / 密碼 `kkbox1234`，並於 OLED 顯示狀態。
- 首次設定：接收機「使用者設定」取得 **裝置連結 Token**；遙控器「網路設定」填入接收機 **IP + Token**。

### 5. 之後的更新：OTA

- 瀏覽器開啟 `http://<裝置IP>/update`，以 `admin / kkbox` 驗證後上傳對應專案的 `.bin`
  （Arduino IDE：`草稿碼 → 匯出已編譯二進位檔`）。
- OTA **只更新程式**，不會更新 SPIFFS 網頁；改網頁仍需回到步驟 3。


### 主要 REST API（兩裝置共通，除註明外）

| 方法     | 路徑                    | 說明                                        |
| -------- | ----------------------- | ------------------------------------------- |
| POST     | `/api/login`            | `user`,`pass` → `{token}`；設 Cookie `sess` |
| POST     | `/api/logout`           | 登出                                        |
| GET      | `/api/status`           | 系統/網路/連線狀態                          |
| GET      | `/api/telemetry`        | 最新飛控遙測                                |
| POST     | `/api/command`          | `cmd`＝JSON 命令字串，轉送飛控板            |
| GET      | `/api/fs`               | SPIFFS 容量與檔案清單                       |
| GET      | `/api/scan`             | WiFi 掃描                                   |
| POST     | `/api/wifi`             | `ssid`,`pass` 或 `action=clear`             |
| GET/POST | `/api/user`             | 帳號設定（POST 需 `cur` 目前密碼）          |
| GET/POST | `/api/net`（僅接收機）  | RJ45 DHCP/固定 IP（儲存後重開機）           |
| GET/POST | `/api/pid`（僅接收機）  | 三軸 PID，儲存後下傳飛控板                  |
| GET/POST | `/api/link`（僅遙控器） | 接收機 IP/埠/Token                          |
| —        | `/update`               | ElegantOTA 上傳頁（HTTP 驗證）              |

> 所有 `/api/*`（除 `login`/`logout`）需帶 Session Cookie 或 `X-Auth` Token，否則回 `401`。

## PID 調校

三軸為 **rate PID**（陀螺儀角速度回授），即 KK 板的「自主平衡」阻尼。調校順序：

1. 固定機體、`cal` 校正後 `arm`，用手輕推機體觀察抵抗方向是否正確；方向反了改 `config.h` 的 `GYRO_REVERSE_*`。
2. 先加 `kp` 直到開始快速震盪，再退回一點。
3. 加 `kd` 抑制震盪；yaw 軸可少量 `ki` 做定角速率。
4. `GYRO_DEG_PER_COUNT` 依實際陀螺儀靈敏度校正，會影響 PID 增益尺度。
