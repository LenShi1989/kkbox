

#define LED1_PORT PORTB
#define LED1_DDR DDRB
#define LED1_BITN PB6

#define LED1_PIN 6

void setup()
{
    // 初始化 LED1
    LED1_DDR |= _BV(LED1_BITN);
    LED1_PORT &= ~_BV(LED1_BITN);

    // 初始化序列埠
    Serial.begin(115200);
    Serial.println("KKmulticontroller V5.5");
    Serial.println("ATmega168 LED1 Test Start");
}

void loop()
{
    // LED1 亮
    LED1_PORT |= _BV(LED1_BITN);
    Serial.println("LED1 ON");
    delay(1000);

    // LED1 滅
    LED1_PORT &= ~_BV(LED1_BITN);
    Serial.println("LED1 OFF");
    delay(1000);
}
