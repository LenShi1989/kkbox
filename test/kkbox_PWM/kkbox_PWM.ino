
/****************************************************
項目	                  設定
開發板	                Arduino Pro or Pro Mini
MCU	                   ATmega168
工作電壓	              3.3 V
時脈	                  8 MHz
Arduino IDE 開發板	    Arduino Pro or Pro Mini
Processor	             ATmega168 (3.3V, 8 MHz)
ESC 訊號腳位	          D9
ESC 控制脈寬	          1000～2000 µs
序列埠鮑率	            9600 bps

---

# 接線方式
## Arduino Pro Mini → ESC

Arduino Pro Mini	  USB-TTL／ESC	   說明
D10	                ESC Signal	     ESC 油門控制訊號
GND	                ESC GND	         共地
D1 TX	              USB-TTL RX       軟體序列埠 TX
D2 RX	              USB-TTL TX	     軟體序列埠 RX
GND	                USB-TTL GND	     通訊共地
*****************************************************/

#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Arduino Pro Mini ATmega168 3.3V / 8MHz
const byte ESC_PIN = 10;

// USB-TTL TX -> D2 (SoftwareSerial RX)
// USB-TTL RX -> D1 (SoftwareSerial TX)

const byte UART_RX_PIN = 2;
const byte UART_TX_PIN = 1;

SoftwareSerial uart(UART_RX_PIN, UART_TX_PIN);
Servo esc;

const int ESC_MIN_US = 1000;
const int ESC_MAX_US = 2000;
const int STEP_US = 10;

int currentUs = ESC_MIN_US;

void setESC(int pulseUs)
{
  currentUs = constrain(pulseUs, ESC_MIN_US, ESC_MAX_US);
  esc.writeMicroseconds(currentUs);

  uart.print(F("ESC Pulse = "));
  uart.print(currentUs);
  uart.println(F(" us"));
}

void printHelp()
{
  uart.println(F("\n=== ESC Test ==="));
  uart.println(F("1000~2000: Set pulse width (us)"));
  uart.println(F("+: Increase 10 us"));
  uart.println(F("-: Decrease 10 us"));
  uart.println(F("s: Minimum throttle 1000 us"));
  uart.println(F("?: Help"));
}

void setup()
{
  uart.begin(9600);
  uart.setTimeout(100);

  esc.attach(ESC_PIN, ESC_MIN_US, ESC_MAX_US);
  setESC(ESC_MIN_US);

  uart.println(F("ESC output initialized to 1000 us."));
  printHelp();
}

void loop()
{
  if (!uart.available())
  {
    return;
  }

  String cmd = uart.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0)
  {
    return;
  }

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
      uart.println(F("Invalid input: enter 1000~2000."));
      return;
    }
  }

  long value = cmd.toInt();

  if (value < ESC_MIN_US || value > ESC_MAX_US)
  {
    uart.println(F("Out of range: enter 1000~2000."));
    return;
  }

  setESC((int)value);
}
