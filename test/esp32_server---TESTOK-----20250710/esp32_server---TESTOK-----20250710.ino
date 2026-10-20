#include <Wire.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);    //SCREEN_HEIGHT = 64,SCREEN_WIDTH = 128, OLED_RESET = -1


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//other uuid pair
//UUID 對照表
//#  SERVICE_UUID                           CHARACTERISTIC_UUID
//1 d973f2a1-1d21-4edb-9ec6-6370c4e8f9a4  e2b8a3c5-5a4e-4c1f-84c9-cf18d531aa4e
//2 f31c7b2d-926e-46f5-8d93-6466b7402493  c9b417e6-0b15-4b37-8d49-36e3da71f4b5
//3 a45f6b13-0c61-4b84-a1b3-3fa5bff1e42d  b1d9efb2-2d4a-45b2-9e27-613e6f4f4772
//4 e1b79cc7-7433-4422-83e0-6b2f087c812a  d398a441-3d0c-4db5-8e42-9a46a5c05b72
//5 ba7e8a45-5157-4ec3-a6fc-56f3bbd9052c  f58d3014-d2f8-4d63-9285-9cd0a37b120b

BLEServer* pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
static unsigned long lastSendTime = 0;
////////////////////////////////////////////////////////////////////////////////////////////
int val_throttle = 0, val_yaw = 0, val_roll = 0, val_pitch = 0, val_vrc1 = 0 , val_vrc2 = 0, val_vrc3 = 0, val_vrc4 = 0 ;
int rx_M1    = 0, rx_M2   = 0, rx_M3  = 0, rx_M4   = 0, rx_M5   = 0, rx_M6  = 0, rx_M7 = 0, rx_M8 = 0;
int rx_pitch = 0, rx_roll = 0, rx_yaw = 0, rx_temp = 0, rx_high = 0, rx_low = 0, rx_sn = 0, rx_sts = 0;

// 類比輸入腳位定義
#define PIN_THROTTLE 39
#define PIN_YAW      35
#define PIN_PITCH    33
#define PIN_ROLL     34
#define PIN_VRC1     13
#define PIN_SW1      10  // 開關1輸入腳位
#define PIN_SW2      18  // 開關2輸入腳位
#define PIN_SW3      19  // 開關3輸入腳位
#define PIN_SW4      23  // 開關4輸入腳位
#define PIN_VRC2     25  // 左翼
#define PIN_VRC3     32  // 右翼
#define PIN_VRC4     12  // 上舵
#define PIN_LED       2  // on board led

volatile bool          isConnected = false;
volatile bool          val_sw1   = false;
volatile bool          val_sw2   = false;
volatile bool          val_sw3   = false;
volatile bool          val_sw4   = false;
volatile bool          rx_state1 = false;
volatile bool          rx_state2 = false;
volatile bool          rx_state3 = false;
volatile bool          ledState  = false;
volatile bool          showState = false;
const unsigned long    show_interval_1s   = 1000;
const unsigned long    show_interval_3s   = 3000;
const unsigned long    reconnect_3s = 3000;
const unsigned long    interval_100ms     = 100;        // 每 100ms 傳送一次
unsigned long          lastDisplayTime = 0;
unsigned long          lastReconnectTime = 0;
//unsigned long          lastSendTime = 0;
unsigned long          lastBlinkTime = 0;
uint8_t                txbuffer[16] = {0};            // 藍芽TX txbuffer區 16 bytes 
uint8_t                rxbuffer[16] = {0};            // 用來儲存對方傳來的資料
int                    switch_display = 0;            // 0 = display rx buffer, 1 = display tx buffer 
int                    fly_id_number = 3;             // fly_id_number無人機id編號(由開機時讀取vrc的值及sw1 = on時決定其0~255無人機編號)
int                    tmax = 2049, tcen = 2048, tmin = 2047;
int                    ymax = 2049, ycen = 2048, ymin = 2047;
int                    pmax = 2049, pcen = 2048, pmin = 2047;
int                    rmax = 2049, rcen = 2048, rmin = 2047;
////////////////////////////////////////////////////////////////////////////////////////
uint8_t   bt_serverno = 3;                // 遙控器(藍芽server模式)編號 0 ~ 255
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; Serial.println("Client connected.");};

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;            Serial.println("Client disconnected.");
    pServer->getAdvertising()->start(); Serial.println("Restart advertising.");
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() <= 16) {
      memcpy(rxbuffer, rxValue.data(), 16);
      //read_rxb_to_var();                            //READ RX BUFFER TO VAR
      rx_M1    = rxbuffer[0];   rx_M2    = rxbuffer[1];
      rx_M3    = rxbuffer[2];   rx_M4    = rxbuffer[3];
      rx_M5    = rxbuffer[4];   rx_M6    = rxbuffer[5];
      rx_M7    = rxbuffer[6];   rx_M8    = rxbuffer[7];
      rx_pitch = rxbuffer[8];   rx_roll  = rxbuffer[9];
      rx_yaw   = rxbuffer[10];  rx_temp  = rxbuffer[11];
      rx_high  = rxbuffer[12];  rx_low   = rxbuffer[13];
      rx_sn    = rxbuffer[14];  rx_sts   = rxbuffer[15]; 
      Serial.print("Server RxDa: ");
      for (int i = 0; i < 16; i++) { Serial.print(rxbuffer[i],DEC); Serial.print("_");}
      Serial.println();
    }
  }
};
/////////////////////////////////////////////////////////////////////////////////////////////////
void read_rxb_to_var(){
  rx_M1    = rxbuffer[0];   rx_M2    = rxbuffer[1];
  rx_M3    = rxbuffer[2];   rx_M4    = rxbuffer[3];
  rx_M5    = rxbuffer[4];   rx_M6    = rxbuffer[5];
  rx_M7    = rxbuffer[6];   rx_M8    = rxbuffer[7];
  rx_pitch = rxbuffer[8];   rx_roll  = rxbuffer[9];
  rx_yaw   = rxbuffer[10];  rx_temp  = rxbuffer[11];
  rx_high  = rxbuffer[12];  rx_low   = rxbuffer[13];
  rx_sn    = rxbuffer[14];  rx_sts   = rxbuffer[15];  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sw1 = 0 then measure min max, other escate measure loop
void mini_max_adjust(){
rt1: 
    val_throttle = analogRead(PIN_THROTTLE);                //0 ~ 4095
    if (val_throttle > tmax){tmax = val_throttle;} 
    if (val_throttle < tmin){tmin = val_throttle;}
       
    val_yaw      = analogRead(PIN_YAW);      
    if (val_yaw > ymax){ymax = val_yaw;} 
    if (val_yaw < ymin){ymin = val_yaw;}
    
    val_pitch    = analogRead(PIN_PITCH);     
    if (val_pitch > pmax){pmax = val_pitch;} 
    if (val_pitch < pmin){pmin = val_pitch;}
          
    val_roll     = analogRead(PIN_ROLL);    
    if (val_roll > rmax){rmax = val_roll;} 
    if (val_roll < rmin){rmin = val_roll;}
   
  val_sw1      = digitalRead(PIN_SW1);     txbuffer[5] = val_sw1;   
  if (txbuffer[5] == 0){                                            //sw1=on continue measure, sw1 = off then quit  
    display.clearDisplay();  display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("tmax:%4d,tmin:%4d\n", tmax, tmin);
    display.setCursor(0, 16);
    display.printf("ymax:%4d,ymin:%4d\n", ymax, ymin);
    display.setCursor(0, 32);
    display.printf("pmax:%4d,pmin:%4d\n", pmax, pmin);
    display.setCursor(0, 48);
    display.printf("rmax:%4d,rmin:%4d\n", rmax, rmin);
    display.display();
    Serial.println("#####################");
    Serial.print("tmax : "); Serial.print(tmax); Serial.print("; tmin : "); Serial.println(tmin);
    Serial.print("ymax : "); Serial.print(ymax); Serial.print("; ymin : "); Serial.println(ymin);
    Serial.print("pmax : "); Serial.print(pmax); Serial.print("; pmin : "); Serial.println(pmin);
    Serial.print("rmax : "); Serial.print(rmax); Serial.print("; rmin : "); Serial.println(rmin);
    Serial.println("---------------------");
    delay(20); 
    goto rt1; 
  }
  Serial.println("-----min_max process finished----------------");  
}
///////////////////////////////////////////////////////////////////////////////////////
void read_analog_sw_to_tx(){  
    // 讀取 ADC 並 map 成 0~255
    val_throttle = analogRead(PIN_THROTTLE);                //less 0 ~ 4095
    txbuffer[0]  = map(val_throttle, tmin, tmax, 0, 255);   //extend to 0~255     
      
    val_yaw      = analogRead(PIN_YAW);                     //less 0 ~ 4095                    
    txbuffer[1]  = map(val_yaw, ymin, ymax, 0, 255);        //extend to 0~255
    
    val_pitch    = analogRead(PIN_PITCH);    
    txbuffer[2]  = map(val_pitch, pmin, pmax, 0, 255);
           
    val_roll     = analogRead(PIN_ROLL);     
    txbuffer[3]  = map(val_roll, rmin, rmax, 0, 255);
    
    val_vrc1     = analogRead(PIN_VRC1);     
    txbuffer[4]  = map(val_vrc1, 0, 4095, 0, 255);    
    val_sw1      = digitalRead(PIN_SW1);     txbuffer[5] = val_sw1 ? 1 : 0;    
    val_sw2      = digitalRead(PIN_SW2);     txbuffer[6] = val_sw2 ? 1 : 0; 
    val_sw3      = digitalRead(PIN_SW3);     txbuffer[7] = val_sw3 ? 1 : 0; 
    val_sw4      = digitalRead(PIN_SW4);     txbuffer[8] = val_sw4 ? 1 : 0; 
    
    val_vrc2     = analogRead(PIN_VRC2);     
    txbuffer[9] = map(val_vrc2, 0, 4095, 0, 255);
     
    val_vrc3     = analogRead(PIN_VRC3);     
    txbuffer[10]= map(val_vrc3, 0, 4095, 0, 255);
    
    val_vrc4     = analogRead(PIN_VRC4);     
    txbuffer[11]= map(val_vrc4, 0, 4095, 0, 255);
    
    txbuffer[12]= bt_serverno;                     //遙控器編號0~255  
    txbuffer[13]= fly_id_number;                   //被遙控無人機id編號0~255
    txbuffer[14]= 1;                               //未定義
    txbuffer[15]= 1;                               //1 = means new data updated
}

//顯示我方txbytes值
void oled_show_txb(){
  display.clearDisplay();  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("THR :%3d, YAW :%3d\n", txbuffer[0], txbuffer[1]);
  display.printf("PIT :%3d, ROL :%3d\n", txbuffer[2], txbuffer[3]);
  display.printf("VRC1:%3d, VRC2:%3d\n", txbuffer[4], txbuffer[9]);
  display.printf("VRC3:%3d, VRC4:%3d\n", txbuffer[10], txbuffer[11]);
  display.printf("SW1 :%s, SW2 :%s\n", txbuffer[5] ? "OFF" : " ON", txbuffer[6] ? "OFF" : " ON");
  display.printf("SW3 :%s, SW4 :%s\n", txbuffer[7] ? "OFF" : " ON", txbuffer[8] ? "OFF" : " ON");
  display.printf("SvNo:%3d, FyID:%3d\n", txbuffer[12], txbuffer[13]);
  display.printf("TB14:%3d, %s", txbuffer[14], txbuffer[15] ? "BT_TX!" : "Non");            //txbuffer[14]未使用
  display.display();
}



//顯示對方rx bytes值       
void oled_show_rxb(){  
  display.clearDisplay();  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("Mot1: %3d, Mot2: %3d\n", rx_M1, rx_M2);      //m1, m2
  display.printf("Mot3: %3d, Mot4: %3d\n", rx_M3, rx_M4);      //m3, m4
  display.printf("Mot5: %3d, Mot6: %3d\n", rx_M5, rx_M6);      //m5, m6
  display.printf("Mot7: %3d, Mot8: %3d\n", rx_M7, rx_M8);      //m7, m8
  display.printf("Pich: %3d, Roll: %3d\n", rx_pitch, rx_roll); //pitch and roll degree 
  display.printf("Yaw : %3d, Temp: %3d\n", rx_yaw, rx_temp);   //yaw and temperature degree
  display.printf("High: %3d, Low : %3d\n", rx_high, rx_low);   //High/LoW byte (飛行高度) +-32768 meter
  display.printf("RxId: %3d, %s\n", rx_sn, rx_sts ? "New Data" : "Old!");
  display.display();
  rxbuffer[15]= 0;     //clear rx status, until rxbuffer[15]= 1 ,then has new updated rx buffer data
}
///////////////////////////////////////////////////////////////////////////////////
void preset_rxb(){
  rxbuffer[0]  = rxbuffer[1] = rxbuffer[2] = rxbuffer[3]= 255;
  rxbuffer[4]  = rxbuffer[5] = rxbuffer[6] = rxbuffer[7]= 128;
  rxbuffer[8]  = rxbuffer[9] = rxbuffer[10] = 2;                 //pitch, roll, yaw angle
  rxbuffer[11] = 24;                                             //temperture = 24 degree
  rxbuffer[12] = 0;             // high byte                                 
  rxbuffer[13] = 140;           // low byte
  rxbuffer[14] = 57;            // rx fly id number 0 ~ 255, typical will be 1,2,3,4.....
  rxbuffer[15] = 1;             //1= ture(has new data), 0 = false(no new data),  
}
//////////////////////////////////////////////////////////////////////////////////////
void set_fly_id_number(){
et1: 
  val_vrc1     = analogRead(PIN_VRC1);     txbuffer[4] = map(val_vrc1, 0, 4095, 0, 255);
  txbuffer[13]  = txbuffer[4];
  fly_id_number = txbuffer[13];
  val_sw1      = digitalRead(PIN_SW1);     txbuffer[5] = val_sw1; 
  
  if (txbuffer[5] != 0){ 
    display.clearDisplay();  display.setTextSize(2);
    display.setCursor(0, 0);
    display.printf("FlyID:%3d\n", fly_id_number);   //m1, m2
    display.setCursor(0, 32);
    display.printf("SW1  : %s\n", txbuffer[5] ? "OFF" : " ON");    
    display.display();
    delay(200); 
    goto et1; 
  }  
  delay(1);    
}
//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  //Wire.begin(5,17);               //WT32   scl=17, sda = 5
  Wire.begin(17,16);                //ESP32  scl=16, sda = 17

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);                         //txt min size  
  display.setTextColor(SSD1306_WHITE);  
  
  //setup led and 4 switch input
  pinMode(PIN_LED, OUTPUT);       digitalWrite(PIN_LED, LOW);
  pinMode(PIN_SW1, INPUT_PULLUP); pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP); pinMode(PIN_SW4, INPUT_PULLUP);
  
  // 設定 ADC 腳位（可選）
  analogReadResolution(12);  // 預設就是 12 位元
///
  Serial.println("Server starting...");
  BLEDevice::init("ESP32_SERVER");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Server is waiting for a client...");
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
////
  set_fly_id_number();                     // vrc + sw1 to set target id number fly
  mini_max_adjust();                       // measure min max of throttle, yaw, pitch, roll
  read_analog_sw_to_tx();                  // read data to tx buffer
  preset_rxb();                            // reset rx buffer data 
  Serial.println("jump to loop");        
}

void loop() {
  // LED 閃爍邏輯：HIGH 300ms, LOW 700ms
  if (millis() - lastBlinkTime >= (ledState ? 300 : 700)) {lastBlinkTime = millis(); ledState = !ledState; digitalWrite(PIN_LED, ledState ? HIGH : LOW);}

  
///  
  if (deviceConnected) {    
    if (!deviceConnected) {delay(1000); return;}         // 等待 client 連線    
    if (millis() - lastSendTime >= 100) {
      lastSendTime = millis();
      read_analog_sw_to_tx();                               //每100ms讀1次8個類比及4switch資料存至txbuffer[0]~bufer[12] 
      Serial.print("Server sent: ");
      for (int i = 0; i < 16; i++) { Serial.print(txbuffer[i],DEC); Serial.print("_");}
      Serial.println();
      pCharacteristic->setValue(txbuffer, 16);
      pCharacteristic->notify();
    }
  }
///    
    if (millis() - lastDisplayTime >= show_interval_3s) {   //show_interval_1s , show_interval_3s     
      lastDisplayTime = millis();        
      if (switch_display == 0){oled_show_rxb(); switch_display = 1;}  //OLED顯示本機[接收]資料
      else                    {oled_show_txb(); switch_display = 0;}  //OLED顯示本機[發射]資料     
      }    
}
///////////////////////////////////////////////////////////////////////////////////////////
