#define LED1_DIRECT 1
#if LED1_DIRECT
#define LED1_PORT PORTB
#define LED1_DDR DDRB
#define LED1_BITN PB6
#else
#define LED1_PIN 13
#endif

uint8_t ledMode = 2; /* 0=off 1=on 2=blink */
uint8_t ledValue = 0;
uint32_t lastLedMs = 0;
bool armed = false;

static inline void ledInit()
{
#if LED1_DIRECT
  LED1_DDR |= _BV(LED1_BITN);
#else
  pinMode(LED1_PIN, OUTPUT);
#endif
}

static inline void ledWrite(uint8_t on)
{
#if LED1_DIRECT
  if (on)
    LED1_PORT |= _BV(LED1_BITN);
  else
    LED1_PORT &= ~_BV(LED1_BITN);
#else
  digitalWrite(LED1_PIN, on ? HIGH : LOW);
#endif
}

/* 非阻塞 LED 顯示: 未解鎖慢閃, 已解鎖快閃 */
static void updateLed()
{
  if (ledMode == 0)
  {
    ledValue = 0;
  }
  else if (ledMode == 1)
  {
    ledValue = 1;
  }
  else
  {
    uint16_t period = armed ? 25 : 500;
    if (millis() - lastLedMs >= period)
    {
      lastLedMs = millis();
      ledValue = !ledValue;
    }
  }
  ledWrite(ledValue);
}

void setup()
{
  ledInit();
}

void loop()
{
  updateLed();
}
