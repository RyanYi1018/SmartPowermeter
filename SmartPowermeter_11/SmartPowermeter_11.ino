
#include <Wire.h>  // Arduino IDE 內建
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <EEPROM.h>

#define BLYNK_PRINT Serial
#include <SoftwareSerial.h>

#include <ESP8266WebServer.h>   //AP模式連線
#include <AutoConnect.h>        //AP模式連線
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>




//SoftwareSerial current_Serial(D3, D4, false, 256); // RX, TX
SoftwareSerial current_Serial;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // 設定 LCD I2C 位址

//D1 SCL
//D2 SDA

#define Relay1 D6
#define Relay2 D7

//紅外線控制
const uint16_t kRecvPin = D5; // IR Recv
const uint16_t kIrLed = D8;  // IR Send

const uint16_t kCaptureBufferSize = 512;

#if DECODE_AC
const uint8_t kTimeout = 50;
#else 
const uint8_t kTimeout = 15;
#endif 

const uint16_t kMinUnknownSize = 12;

uint16_t raw_buf[4][100];
unsigned char * p_raw_buf = (unsigned char *)&raw_buf[0][0];


//虛擬節點設定
#define V_bridge V100

#define V_today_date V0
#define V_today_time V1

//負載1
#define V_load1_state V2
#define V_load1_kwh_limit_setting V4
#define V_load1_timer_setting V5
#define V_load1_Ir_ON V6
#define V_load1_Ir_OFF V7
#define V_load1_control_choice V8
#define V_load1_autooff_time_choice V9

#define V_load1_acc_kwh1 V3
#define V_now_current1 V15
#define V_now_power1 V16
#define V_led1 V13

#define V_smarthand_on V10
#define V_smarthand_off V11

//負載2
#define V_load2_state V22
#define V_load2_kwh_limit_setting V24
#define V_load2_timer_setting V25
#define V_load2_Ir_ON V26
#define V_load2_Ir_OFF V27
#define V_load2_control_choice V28
#define V_load2_autooff_time_choice V29

#define V_load2_acc_kwh2 V23
#define V_now_current2 V35
#define V_now_power2 V36
#define V_led2 V14

//紅外線
#define V_IR_state V40
#define V_load1_IR_ON_btn V45
#define V_load1_IR_OFF_btn V47
#define V_load1_IR_ON_learn_btn V46
#define V_load1_IR_OFF_learn_btn V48

#define V_IR_state V40
#define V_load2_IR_ON_btn V49
#define V_load2_IR_OFF_btn V51
#define V_load2_IR_ON_learn_btn V50
#define V_load2_IR_OFF_learn_btn V52
#define V_led3 V55

#define V_now_meter_IP V53
#define V_now_hand_IP V54

//小手
#define V_hand1_on V2
#define V_hand1_off V3
#define V_hand2_on V4
#define V_hand2_off V5

//全域變數宣告
byte cmmd[20];
int insize;
double now_current1=0, now_current2=0;
double acc_kwh1=0,acc_kwh2=0;
double now_power1=0,now_power2=0;
int led_flag=0;
int switch1 = 0;
int switch2 = 0;
int now_timecounter = 0;
int load1_kwh_limit_setting = 0; 
int load1_timer_setting = 0;
int load1_nowtimer = 0;
int load2_kwh_limit_setting = 0;
int load2_timer_setting = 0;
int load2_nowtimer = 0;
int load1_control_method = 1;
int load2_control_method = 1;

// WiFi Parameters
const char* ssid = "";
const char* password = "";
char auth_esp8266_meter[] = "Gx3dQe7gHtftiVqD3r_6PbOqVj6gVDI1";
char auth_esp8266_hand[] = "9ypf1BtWnmp3YkPxc45CU8Wtu8JwzOzL";

/*char auth_esp8266_meter[] = "eqz2-f1gzgJD6qy9kzHiC-1lNdT093Fd";
char auth_esp8266_hand[] = "I7NDTZfu6TTg2FzL5-_8ZnYuEVS44nBj";*/

//const char* ssid = "TP-LINK_4F";
//const char* password = "0000000000";
//char auth[] = "XXX"; //你們的

String week_name[] = {"","日","一","二","三","四","五","六",};

//---- AP模式連線 -----
ESP8266WebServer Server;
AutoConnect Auto_Connect(Server);
AutoConnectConfig Auto_Connect_config;
//--------------------

//物件宣告
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
IRsend irsend(kIrLed);
decode_results results;  // Somewhere to store the results
BlynkTimer timer;
WidgetRTC rtc;
WidgetLED led1(V_led1);
WidgetLED led2(V_led2);
WidgetLED led3(V_led3);
WidgetBridge bridge_hand(V_bridge);//V為一個與Server的通道，兩塊板子不一定要一樣

uint16_t * get_rawint_code(const decode_results * const p_results, uint16_t * raw_buf);


void rootPage() { //AP模式連線
  char content[] = "System Starting...";  //顯示系統訊息
  Server.send(200, "text/plain", content);
}

void system_led(){
  
  led_flag++;
  
  if((led_flag % 2)==1){
    led1.on();    
    led2.on();
    led3.on();       
  }else{
    led1.off();    
    led2.off(); 
    led3.off();       
  }
}

void clockDisplay()
{
  String currentTime = String(hour()) + "時 " + minute() + "分 " + second()+ "秒";
  String currentDate = String(year()) + "年 " + month() + "月 " + day() + "日 " + "星期" + week_name[weekday()];

  now_timecounter++;
  Blynk.virtualWrite(V_now_meter_IP, WiFi.localIP().toString());
  
//---sweitch1------  
  if(switch1 == 2){
    load1_nowtimer++;
    if(load1_nowtimer == load1_timer_setting){
      if((load1_control_method == 3)||(load1_control_method == 4)||(load1_control_method == 5))
        bridge_hand.virtualWrite(V_hand1_off, 1);//智慧小手 ON_V2   OFF_V3
        digitalWrite(Relay1, HIGH); 
    }
  }
  
//---sweitch2------
  if(switch2 == 2){
    load2_nowtimer++;
    if(load2_nowtimer == load2_timer_setting){
      if((load2_control_method == 3)||(load2_control_method == 4)||(load2_control_method == 5))
        bridge_hand.virtualWrite(V_hand2_off, 1);//智慧小手 ON_V2   OFF_V3
      digitalWrite(Relay2, HIGH); 
    }
  }

  if(load1_kwh_limit_setting <= acc_kwh1){
      if((load1_control_method == 3)||(load1_control_method == 4)||(load1_control_method == 5))
        bridge_hand.virtualWrite(V_hand1_off, 1);//智慧小手 ON_V2   OFF_V3
      digitalWrite(Relay1, HIGH);    
  }

  if(load2_kwh_limit_setting <= acc_kwh2){
      if((load2_control_method == 3)||(load2_control_method == 4)||(load2_control_method == 5))
        bridge_hand.virtualWrite(V_hand2_off, 1);//智慧小手 ON_V2   OFF_V3    
      digitalWrite(Relay2, HIGH);    
  }
 
  Blynk.virtualWrite(V_today_date, currentDate);
  Blynk.virtualWrite(V_today_time, currentTime);
}

void current_read(){
  
   if((insize=(current_Serial.available()))>0){
        //Serial.print("input size = "); 
        //Serial.println(insize);
        for (int i=0; i<insize; i++){
          cmmd[i] = current_Serial.read();
          //Serial.print(String(i) + ':');Serial.print(cmmd[i]=current_Serial.read());
          //Serial.print("\n"); 
        }
     }    
    if(cmmd[0] == 97){
      
      now_current1 = (cmmd[1]*256 + cmmd[2])*(1)/ 1000.0;
      now_power1 = now_current1 * 110;
      acc_kwh1 = acc_kwh1 + (now_power1 / 3.6);

      now_current2 = (cmmd[3]*256 + cmmd[4])*(1) / 1000.0;
      now_power2 = now_current2 * 110;
      acc_kwh2 = acc_kwh2 + (now_power2 / 3.6);
      
      Serial.print("Current Value 1 = ");
      Serial.println(now_current1);
      Serial.print("acc_kwh Value 1 = ");
      Serial.println(acc_kwh1);
      
      Serial.print("Current Value 2 = ");
      Serial.println(now_current2);
      Serial.print("acc_kwh Value 2 = ");
      Serial.println(acc_kwh2);
           
      if (now_current1 > 0.05)
        Blynk.virtualWrite(V_load1_state,String("<1 使用中>"));
      else
        Blynk.virtualWrite(V_load1_state,String("<1 停止>")); 

      if (now_current2 > 0.05)
        Blynk.virtualWrite(V_load2_state,String("<2 使用中>"));
      else
        Blynk.virtualWrite(V_load2_state,String("<2 停止>"));
      
      Blynk.virtualWrite(V_now_current1,String(now_current1));  //瞬時電流1     
      Blynk.virtualWrite(V_now_power1,String(now_power1)); //瞬時功率1
      Blynk.virtualWrite(V_load1_acc_kwh1,String(acc_kwh1)); //累計電度1 

      Blynk.virtualWrite(V_now_current2,String(now_current2));  //瞬時電流2      
      Blynk.virtualWrite(V_now_power2,String(now_power2)); //瞬時功率2
      Blynk.virtualWrite(V_load2_acc_kwh2,String(acc_kwh2)); //累計電度2 

      lcd.setCursor(0, 0);
      lcd.print("                ");            
      lcd.setCursor(0, 1);
      lcd.print("                ");  
      lcd.setCursor(0, 0);
      lcd.print(String("L1:") + now_current1 + "A," + now_power1 + "W");            
      lcd.setCursor(0, 1);
      lcd.print(String("L2:") + now_current2 + "A," + now_power2 + "W");         

    }
}


BLYNK_WRITE(V_load1_control_choice)//負載1 控制方法 1:一般，2:紅外線，3:智慧小手
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  switch (pinValue) {
    case 1://一般
    {
      load1_control_method = 1;
      Blynk.virtualWrite(V_load1_state,String("<1 一般控制>"));
    }
    break;
    case 2://紅外線
    {
      load1_control_method = 2;  
      Blynk.virtualWrite(V_load1_state,String("<1 紅外線控制>"));    
    }
    break;    
    case 3://智慧小手(即時)
    {
      load1_control_method = 3;
      Blynk.virtualWrite(V_load1_state,String("<1 智慧小手>"));     
    }
    break;  
    case 4://智慧小手(延後20秒)
    {
      load1_control_method = 4;
      Blynk.virtualWrite(V_load1_state,String("<1 智慧小手>"));     
    }
    break;    
    case 5://智慧小手(延後40秒)
    {
      load1_control_method = 5;
      Blynk.virtualWrite(V_load1_state,String("<1 智慧小手>"));     
    }
    break;         
  }
}

BLYNK_WRITE(V_load2_control_choice)//負載2 控制方法 1:一般，2:紅外線，3:智慧小手
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  switch (pinValue) {
    case 1://一般
    {
      load2_control_method = 1;
      Blynk.virtualWrite(V_load2_state,String("<2 一般控制>"));
    }
    break;
    case 2://紅外線
    {
      load2_control_method = 2;  
      Blynk.virtualWrite(V_load2_state,String("<2 紅外線控制>"));    
    }
    break;    
    case 3://智慧小手(即時)
    {
      load2_control_method = 3;
      Blynk.virtualWrite(V_load2_state,String("<2 智慧小手>"));     
    }
    break;  
    case 4://智慧小手(延後20秒)
    {
      load2_control_method = 4;
      Blynk.virtualWrite(V_load2_state,String("<2 智慧小手>"));     
    }
    break;
    case 5://智慧小手(延後40秒)
    {
      load2_control_method = 5;
      Blynk.virtualWrite(V_load2_state,String("<2 智慧小手>"));     
    }
    break;         
  }
}

BLYNK_WRITE(V_load1_autooff_time_choice)//switch1 切換
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  switch (pinValue) {
    case 1://手動
    {
      switch1 = 1;
      Blynk.virtualWrite(V_load1_state,String("<1 手動>"));
    }
    break;
    case 2://計時
    {
      switch1 = 2;  
      load1_nowtimer = 0;
      Blynk.virtualWrite(V_load1_state,String("<1 計時>"));    
    }
    break;    
    case 3://清除kwh
    {
      switch1 = 2;
      Blynk.virtualWrite(V_load1_state,String("<1 kwh清除>")); 
      acc_kwh1 = 0;     
    }
    break;    
  }
}

BLYNK_WRITE(V_load2_autooff_time_choice)//switch2 切換
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  switch (pinValue) {
    case 1://手動
    {
      switch2 = 1;
      Blynk.virtualWrite(V_load2_state,String("<2 手動>"));
    }
    break;
    case 2://計時
    {
      switch2 = 2;  
      load2_nowtimer = 0;
      Blynk.virtualWrite(V_load2_state,String("<2 計時>"));    
    }
    break;    
    case 3://清除kwh
    {
      switch2 = 2;
      Blynk.virtualWrite(V_load2_state,String("<2 kwh清除>")); 
      acc_kwh2 = 0;     
    }
    break;    
  }
}

BLYNK_WRITE(V_load1_Ir_ON)//負載1 啟動
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  if(pinValue == 1){
    
    digitalWrite(Relay1, LOW);
    delay(1500);
    
    if((load1_control_method == 3)||(load1_control_method == 4)||(load1_control_method == 5))
       bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3

                    
    if(load1_control_method == 2){
      delay(1000);
      irsend.sendRaw(&raw_buf[0][1], raw_buf[0][0]-1, 38);  // Send a raw data capture at 38kHz. 
      
      Blynk.virtualWrite(V_IR_state, "<1>負載1 Sending ..OK");  
    }
    
    load1_nowtimer=0; 
  }

}

BLYNK_WRITE(V_load1_Ir_OFF)//負載1 停止
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  if(pinValue == 1){
    
    if(load1_control_method == 3){//即時
       bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3      
       delay(1000);
    }
    if(load1_control_method == 4){//延後20秒
       bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3      
       //delay(1000);
       delay(20000);       
    }
    if(load1_control_method == 5){//延後40秒
       bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3      
       //delay(1000);
       delay(40000);
    }   

    if(load1_control_method == 2){
      irsend.sendRaw(&raw_buf[1][1], raw_buf[1][0]-1, 38);  // Send a raw data capture at 38kHz.
      Blynk.virtualWrite(V_IR_state, "<2>負載1 Sending ..OK"); 
      delay(2500);
    }
    
    //delay(2000);      
    digitalWrite(Relay1, HIGH); 
  }
}

BLYNK_WRITE(V_load2_Ir_ON)//負載2 啟動
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  if(pinValue == 1){
    digitalWrite(Relay2, LOW); 
    delay(1500);
    if((load2_control_method == 3)||(load2_control_method == 4)||(load2_control_method == 5))
       bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3
            
    if(load2_control_method == 2){
      delay(1000);
      irsend.sendRaw(&raw_buf[0][1], raw_buf[0][0]-1, 38);  // Send a raw data capture at 38kHz. 
         
    load2_nowtimer=0;     
  }

 }
}

BLYNK_WRITE(V_load2_Ir_OFF)//負載2 停止
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  
  if(pinValue == 1){ 

    if(load2_control_method == 3){//即時
        bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3
        delay(1000);   
        //delay(20000);  
    }
    if(load2_control_method == 4){//延後20秒
        bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3
        //delay(1000);   
        delay(20000);  
    }
    if(load2_control_method == 5){//延後40秒
        bridge_hand.virtualWrite(V_hand1_on, 1);//智慧小手 ON_V2   OFF_V3
        //delay(1000);   
        delay(40000);  
    }


    if(load2_control_method == 2){
      irsend.sendRaw(&raw_buf[1][1], raw_buf[1][0]-1, 38);  // Send a raw data capture at 38kHz.
      Blynk.virtualWrite(V_IR_state, "<2>負載2 Sending ..OK"); 
      delay(2500); 
    }
        
    digitalWrite(Relay2, HIGH); 
  }
}


BLYNK_WRITE(V_load1_kwh_limit_setting)
{
  int pinValue = param.asInt();
  load1_kwh_limit_setting = pinValue;
}

BLYNK_WRITE(V_load1_timer_setting)
{
  int pinValue = param.asInt();
  load1_timer_setting = pinValue;
}

BLYNK_WRITE(V_load2_kwh_limit_setting)
{
  int pinValue = param.asInt();
  load2_kwh_limit_setting = pinValue;
}

BLYNK_WRITE(V_load2_timer_setting)
{
  int pinValue = param.asInt();
  load2_timer_setting = pinValue;
}

BLYNK_WRITE(V_smarthand_on)
{
  int pinValue = param.asInt();

  if(pinValue == 1){
    bridge_hand.virtualWrite(V2, 1);//智慧小手 ON_V2   OFF_V3
  }
}

BLYNK_WRITE(V_smarthand_off)
{
  int pinValue = param.asInt();

  if(pinValue == 1){
    bridge_hand.virtualWrite(V3, 1);//智慧小手 OFF_V3 ON_V2   
  }
}

BLYNK_WRITE(V_load1_IR_ON_learn_btn)//負載1_紅外線--紀錄1
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue ==1){
    
    Serial.println("on 負載1紅外線紀錄<1>開始...");
    delay(200);
    if (irrecv.decode(&results)) {
      get_rawint_code(&results, &raw_buf[0][0]);
    }  
         
  }else{
    
    Serial.println("off 負載1列印紅外線編碼<1>");
    Serial.println("Int RAW:");
    
    for(unsigned int j=0; j<raw_buf[0][0]; j++){
      Serial.print(raw_buf[0][j]);   
      Serial.print(", ");
    }
    Blynk.virtualWrite(V_IR_state, String(raw_buf[0][0])+" Byte!<1>");
    Serial.println("<1>負載1 Reading....OK");       
  }
}

BLYNK_WRITE(V_load1_IR_OFF_learn_btn)//負載1_紅外線--紀錄2
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue ==1){
    
    Serial.println("on 負載1紅外線紀錄<2>開始...");
    delay(200);
    if (irrecv.decode(&results)) {
      get_rawint_code(&results, &raw_buf[1][0]);
    }  
         
  }else{
    
    Serial.println("off 負載1列印紅外線編碼<2>");
    Serial.println("Int RAW:");
    
    for(unsigned int j=0; j<raw_buf[1][0]; j++){
      Serial.print(raw_buf[1][j]);   
      Serial.print(", ");
    }
    Blynk.virtualWrite(V_IR_state, String(raw_buf[1][0])+" Byte!<2>");
    Serial.println("<2>負載1 Reading....OK");       
  }
}

BLYNK_WRITE(V_load1_IR_ON_btn)//負載1_紅外線--發射1
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue == 1){
    irsend.sendRaw(&raw_buf[0][1], raw_buf[0][0]-1, 38);  // Send a raw data capture at 38kHz. 
    Blynk.virtualWrite(V_IR_state, "<1>負載1 Sending ..OK");  
  } 
}
BLYNK_WRITE(V_load1_IR_OFF_btn)//負載1_紅外線--發射2
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue == 1){
    irsend.sendRaw(&raw_buf[1][1], raw_buf[1][0]-1, 38);  // Send a raw data capture at 38kHz. 
    Blynk.virtualWrite(V_IR_state, "<2>負載1 Sending ..OK");  
  } 
}

BLYNK_WRITE(V_load2_IR_ON_learn_btn)//負載2_紅外線--紀錄1
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue ==1){
    
    Serial.println("on 負載2紅外線紀錄<1>開始...");
    delay(200);
    if (irrecv.decode(&results)) {
      get_rawint_code(&results, &raw_buf[0][0]);
    }  
         
  }else{
    
    Serial.println("off 負載2列印紅外線編碼<1>");
    Serial.println("Int RAW:");
    
    for(unsigned int j=0; j<raw_buf[0][0]; j++){
      Serial.print(raw_buf[0][j]);   
      Serial.print(", ");
    }
    Blynk.virtualWrite(V_IR_state, String(raw_buf[0][0])+" Byte!<1>");
    Serial.println("<1>負載2 Reading....OK");       
  }
}

BLYNK_WRITE(V_load2_IR_OFF_learn_btn)//負載2_紅外線--紀錄2
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue ==1){
    
    Serial.println("on 負載2紅外線紀錄<2>開始...");
    delay(200);
    if (irrecv.decode(&results)) {
      get_rawint_code(&results, &raw_buf[1][0]);
    }  
         
  }else{
    
    Serial.println("off 負載2列印紅外線編碼<2>");
    Serial.println("Int RAW:");
    
    for(unsigned int j=0; j<raw_buf[1][0]; j++){
      Serial.print(raw_buf[1][j]);   
      Serial.print(", ");
    }
    Blynk.virtualWrite(V_IR_state, String(raw_buf[1][0])+" Byte!<2>");
    Serial.println("<2>負載2 Reading....OK");       
  }
}

BLYNK_WRITE(V_load2_IR_ON_btn)//負載2_紅外線--發射1
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue == 1){
    irsend.sendRaw(&raw_buf[0][1], raw_buf[0][0]-1, 38);  // Send a raw data capture at 38kHz. 
    Blynk.virtualWrite(V_IR_state, "<1>負載2 Sending ..OK");  
  } 
}
BLYNK_WRITE(V_load2_IR_OFF_btn)//負載2_紅外線--發射2
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable

  if(pinValue == 1){
    irsend.sendRaw(&raw_buf[1][1], raw_buf[1][0]-1, 38);  // Send a raw data capture at 38kHz. 
    Blynk.virtualWrite(V_IR_state, "<2>負載2 Sending ..OK");  
  } 
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V_load1_Ir_ON);
  Blynk.syncVirtual(V_load1_Ir_OFF);
  Blynk.syncVirtual(V_load2_Ir_ON);
  Blynk.syncVirtual(V_load2_Ir_OFF);
  
  Blynk.syncVirtual(V_load1_kwh_limit_setting); 
  Blynk.syncVirtual(V_load1_timer_setting);
  Blynk.syncVirtual(V_load2_kwh_limit_setting); 
  Blynk.syncVirtual(V_load2_timer_setting);
  Blynk.syncVirtual(V_load1_autooff_time_choice);    
  Blynk.syncVirtual(V_load2_autooff_time_choice);
}

void setup() {

  Serial.begin(115200);
  //current_Serial.begin(9600);
  current_Serial.begin(9600, SWSERIAL_8N1, D3, D4, false, 256); // RX, TX
  
//----- AP模連線設定參數 ------
  Auto_Connect_config.apid = String("電力監控");  //AP模WIFI連線帳號
  Auto_Connect_config.psk = String("123456789"); //AP模WIFI連線密碼
  Auto_Connect.config(Auto_Connect_config);
  
  Server.on("/", rootPage);
  if (Auto_Connect.begin()) {  //30秒無法上網，自動啟動AP模式
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
//-------------------------

  Blynk.begin(auth_esp8266_meter, ssid, password,"192.168.43.22",8080);
  //Blynk.begin(auth_esp8266_meter, ssid, password,"blynk.iot-cm.com",8080);

  bridge_hand.setAuthToken(auth_esp8266_hand);//對方Token
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting...");
  }

  lcd.begin(16, 2);
  lcd.backlight();
  rtc.begin();

  irrecv.enableIRIn();  // Start the receiver
  irsend.begin();
  
  pinMode(Relay1, OUTPUT);digitalWrite(Relay1, HIGH);
  pinMode(Relay2, OUTPUT);digitalWrite(Relay2, HIGH);
  
  timer.setInterval(1000L, current_read);
  timer.setInterval(1000L, clockDisplay); 
  timer.setInterval(1000L, system_led); 
  
}


void loop() {
  Blynk.run();
  timer.run();
  Auto_Connect.handleClient(); //AP模式連線  
}

//-----------------
uint16_t * get_rawint_code(const decode_results * const results, uint16_t * buf) {

  *(buf + 0) = results->rawlen;

  for (uint16_t i = 1; i < results->rawlen; i++) {
    uint32_t usecs;
    for (usecs = results->rawbuf[i] * kRawTick; usecs > UINT16_MAX;usecs -= UINT16_MAX) {
      *(buf + i) = UINT16_MAX;
    }
      *(buf + i) = usecs;

  }
  return buf;
}
