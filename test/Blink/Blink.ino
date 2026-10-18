#define LED1_PIN 6

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  Serial.println("okok");
  digitalWrite(LED1_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED1_PIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
