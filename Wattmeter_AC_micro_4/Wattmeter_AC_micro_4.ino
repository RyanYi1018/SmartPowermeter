
#include "ACS712.h"
#include <SoftwareSerial.h>

SoftwareSerial current_Serial(3, 2); // RX, TX

ACS712 sensor1(ACS712_30A, A3);
ACS712 sensor2(ACS712_30A, A2);

float zero_point1;
float zero_point2;


unsigned long previousMillis = 0;

#define FILTER_N 50

float Filter1() {//中位值平均濾波法
  int i, j;
  float filter_temp, filter_sum = 0;
  float filter_buf[FILTER_N];
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = sensor1.getCurrentAC(60) *1000.0 *1 / 1;
    delay(2);
  }
  // 采樣值從小到大排列（冒泡法）
  for(j = 0; j < FILTER_N - 1; j++) {
    for(i = 0; i < FILTER_N - 1 - j; i++) {
      if(filter_buf[i] > filter_buf[i + 1]) {
        filter_temp = filter_buf[i];
        filter_buf[i] = filter_buf[i + 1];
        filter_buf[i + 1] = filter_temp;
      }
    }
  }
  // 去除最大最小極值後求平均
  for(i = 1; i < FILTER_N - 1; i++) filter_sum += filter_buf[i];
  return filter_sum / (FILTER_N - 2);
}

float Filter2() {//中位值平均濾波法
  int i, j;
  float filter_temp, filter_sum = 0;
  float filter_buf[FILTER_N];
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = sensor2.getCurrentAC(60) *1000 *1 /1;
    delay(2);
  }
  // 采樣值從小到大排列（冒泡法）
  for(j = 0; j < FILTER_N - 1; j++) {
    for(i = 0; i < FILTER_N - 1 - j; i++) {
      if(filter_buf[i] > filter_buf[i + 1]) {
        filter_temp = filter_buf[i];
        filter_buf[i] = filter_buf[i + 1];
        filter_buf[i + 1] = filter_temp;
      }
    }
  }
  // 去除最大最小極值後求平均
  for(i = 1; i < FILTER_N - 1; i++) filter_sum += filter_buf[i];
  return filter_sum / (FILTER_N - 2);
}

void setup() {
  Serial.begin(9600);
  current_Serial.begin(9600);
  
  delay(2000);
  
  zero_point1 = sensor1.calibrate();  //開機時，裝置必須全部關閉
  Serial.print("Zero point for this sensor1 is ");
  Serial.println(zero_point1);
  sensor1.setZeroPoint(zero_point1);

  delay(2000);

  zero_point2 = sensor2.calibrate();  //開機時，裝置必須全部關閉
  Serial.print("Zero point for this sensor2 is ");
  Serial.println(zero_point2);
  sensor2.setZeroPoint(zero_point2);

}

void loop() {

//----AC1--------
  sensor1.setZeroPoint(zero_point1);

  float Filter_I1 = Filter1();
  
//----AC2--------
  sensor2.setZeroPoint(zero_point2);

  float Filter_I2 = Filter2();

//--------------
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000) {
    
      previousMillis = currentMillis;

      if(Filter_I1<80) Filter_I1 = 0;
      Serial.println(String("current 1:") + (int)Filter_I1);
      unsigned int send_Data1 = (unsigned int)Filter_I1;
      
      if(Filter_I2<80) Filter_I2 = 0;
      Serial.println(String("current 2:") + (int)Filter_I2);
      unsigned int send_Data2 = (unsigned int)Filter_I2;
  
    //-----Send-------  
      byte packet[5]; 
      
      packet[0] = 97;
      
      packet[1] = send_Data1 / 256;
      packet[2] = send_Data1 % 256;
      
      packet[3] = send_Data2 / 256;
      packet[4] = send_Data2 % 256;
       
      for(int i = 0; i < 5; i++) 
        current_Serial.write(packet[i]);
    
  }

}
