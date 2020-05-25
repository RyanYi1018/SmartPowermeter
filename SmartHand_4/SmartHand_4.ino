
#include <Wire.h>
#define BLYNK_PRINT Serial
#include <ESP8266WebServer.h>   //AP模式連線
#include <AutoConnect.h>        //AP模式連線
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <FastLED.h>

#define V_bridge V100
#define V_today_date V0
#define V_today_date V1



//char auth_esp32[] = "";
//char auth_esp8266[] = ""; 
//char ssid[] = "AP16DC";
//char pass[] = "qyxe3265";

/*char auth_esp8266_meter[] = "1_z0shBlcSQ7z9A_E_vUNvzu3074FDFK";
char auth_esp8266_hand[] = "najpjhdqfoYh7YqMHfoRyGUynJJyBPjo";
char ssid[] = "";
char pass[] = "";*/
char auth_esp8266_meter[] = "Gx3dQe7gHtftiVqD3r_6PbOqVj6gVDI1";
char auth_esp8266_hand[] = "9ypf1BtWnmp3YkPxc45CU8Wtu8JwzOzL";
char ssid[] = "";
char pass[] = "";

//---- AP模式連線 -----
ESP8266WebServer Server;
AutoConnect Auto_Connect(Server);
AutoConnectConfig Auto_Connect_config;
//--------------------

BlynkTimer timer;
WidgetRTC rtc;
WidgetBridge bridge_meter(V_bridge);//V為一個與Server的通道，兩塊板子不一定要一樣
Servo myservo;

#define NUM_LEDS 1
#define DATA_PIN D3
CRGB leds[NUM_LEDS];

int pos = 0;
int servoPin = D4;


void rootPage() { //AP模式連線
  char content[] = "System Starting...";  //顯示系統訊息
  Server.send(200, "text/plain", content);
}

BLYNK_WRITE(V2) // bridge 智慧小手開啟
{
  int pinValue = param.asInt();
  
  if(pinValue == 1){
      leds[0] = CRGB::Red;FastLED.show();
      myservo.write(280);delay(100);
      myservo.write(180);delay(100);
            myservo.write(90);delay(100);
      delay(500);   
      leds[0] = CRGB::Green;FastLED.show();
  }
}

BLYNK_WRITE(V3) // bridge 智慧小手關閉
{
  int pinValue = param.asInt();
  
  if(pinValue == 1){    
      leds[0] = CRGB::Green;FastLED.show();
      myservo.write(90);
      delay(500);
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V3); //小手先關閉 
}

void setup() {

  Serial.begin(9600);

//----- AP模連線設定參數 ------
  Auto_Connect_config.apid = String("智慧小手");  //AP模WIFI連線帳號
  Auto_Connect_config.psk = String("123456789"); //AP模WIFI連線密碼
  Auto_Connect.config(Auto_Connect_config);
  
  Server.on("/", rootPage);
  if (Auto_Connect.begin()) {  //30秒無法上網，自動啟動AP模式
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
//-------------------------

  //Blynk.begin(auth_esp8266_hand, ssid, pass,"blynk.iot-cm.com",8080);
  Blynk.begin(auth_esp8266_hand, ssid, pass,"192.168.43.22",8080);

  bridge_meter.setAuthToken(auth_esp8266_meter);//對方Token
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting...");
  }

  bridge_meter.virtualWrite(V54, String(WiFi.localIP().toString()));

  rtc.begin();
  
  myservo.attach(servoPin);
  myservo.write(90);
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB 為 紅綠藍
  FastLED.setBrightness(20); //亮度設定 0-255
  set_max_power_in_volts_and_milliamps(5, 500); // 5V, 500mA
  leds[0] = CRGB::Green;FastLED.show();
  
  //timer.setInterval(3000L, test);
  
}


void loop() {
  Blynk.run();
  timer.run();
  Auto_Connect.handleClient(); //AP模式連線  
}

//
