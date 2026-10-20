/****************************************************
ESP32 D1 mini	  連接對象	        功能
GPIO 18	        驅動器 PWM／EN	  PWM 速度控制
GND	            驅動器 GND	      共地
USB	            電腦	            程式燒錄與序列通訊
外部電源	       馬達驅動器電源端   提供馬達電力

---

# 測試規格
項目	          設定
開發板	        ESP32 D1 mini
開發環境	      Arduino IDE
ESC 訊號腳位	  GPIO 18
PWM 頻率	      50 Hz
最低油門	      1000 µs
最高油門	      2000 µs
測試步進	      每次增加 10 µs
控制方式	      序列埠輸入脈寬
RPM 量測	      需額外轉速感測器

****************************************************/

#include <Arduino.h>
#include <esp_arduino_version.h>

const int ESC_PIN = 18;
const int ESC_FREQ = 50;
const int PWM_BITS = 14;

const int ESC_MIN_US = 1000;
const int ESC_MAX_US = 2000;
const int STEP_US = 10;

const uint32_t PERIOD_US = 1000000UL / ESC_FREQ;
const uint32_t PWM_MAX = (1UL << PWM_BITS) - 1;

int currentUs = ESC_MIN_US;

uint32_t pulseToDuty(int pulseUs)
{
  return (uint32_t)(((uint64_t)pulseUs * PWM_MAX + PERIOD_US / 2) / PERIOD_US);
}

void setESC(int pulseUs)
{
  currentUs = constrain(pulseUs, ESC_MIN_US, ESC_MAX_US);
  uint32_t duty = pulseToDuty(currentUs);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(ESC_PIN, duty);
#else
  ledcWrite(0, duty);
#endif

  Serial.printf(
      "油門脈寬：%d us | 占空比：%.1f%%\n",
      currentUs,
      currentUs * 100.0 / PERIOD_US);
}

void printHelp()
{
  Serial.println("\n=== ESP32 ESC 馬達測試 ===");
  Serial.println("輸入 1000~2000：設定油門脈寬");
  Serial.println("+：增加 10 us");
  Serial.println("-：減少 10 us");
  Serial.println("s：回到最低油門 1000 us");
  Serial.println("?：顯示說明");
  Serial.println("注意：此程式不會直接量測 RPM。");
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(ESC_PIN, OUTPUT);
  digitalWrite(ESC_PIN, LOW);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if (!ledcAttach(ESC_PIN, ESC_FREQ, PWM_BITS))
  {
    Serial.println("錯誤：PWM 初始化失敗");
    while (true)
    {
      delay(1000);
    }
  }
#else
  ledcSetup(0, ESC_FREQ, PWM_BITS);
  ledcAttachPin(ESC_PIN, 0);
#endif

  setESC(ESC_MIN_US);

  Serial.println("請確認 ESC 已完成最低油門辨識。");
  printHelp();
}

void loop()
{
  if (!Serial.available())
    return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0)
    return;

  if (cmd == "s" || cmd == "S")
  {
    setESC(ESC_MIN_US);
    return;
  }

  if (cmd == "+")
  {
    setESC(currentUs + STEP_US);
    return;
  }

  if (cmd == "-")
  {
    setESC(currentUs - STEP_US);
    return;
  }

  if (cmd == "?")
  {
    printHelp();
    return;
  }

  for (unsigned int i = 0; i < cmd.length(); i++)
  {
    if (!isDigit(cmd.charAt(i)))
    {
      Serial.println("輸入錯誤，請輸入 1000~2000。");
      return;
    }
  }

  long value = cmd.toInt();

  if (value < ESC_MIN_US || value > ESC_MAX_US)
  {
    Serial.println("超出範圍，請輸入 1000~2000。");
    return;
  }

  setESC((int)value);
}
