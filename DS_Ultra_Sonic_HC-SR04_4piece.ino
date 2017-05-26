#include <EEPROM.h>
//#include <MCP23S17.h>

//********************************************************************
//*超音波センサを使って距離を表示するプログラム
//********************************************************************

//sensor maping (1,2,3,4) = (left_side, left_upper, right_upper, right_side) 
/*
以下実装予定↓
●センサキャリブレーション
left_side, right_side　デフォルトは無限遠＝通常時はセンサ値取れない（近すぎて取れない場合と判別がつかないため注意！）
left_upper, right_upper デフォルトはキャリブレーションされた地面までの距離。反応時の距離を見ることでアラートを出すか判別可能（反応しても近距離の場合はOK)
１：キャリブレーションボタン押下→キャリブレーションモード
２：ボタン押下時の各センサの値をEEPROMに保存、以降この値を参照して配達員の検知
３：シリアルプリント時に常にキャリブレーションの値を返す（いつでも確認できるように）
●センサ反応時スピーカーから警告音を出す
●mcp23s17を利用してIOを増やし、接続センサ数を増やす
 */

#define echoPin_1  2 // Echo Pin@ sensor 1 //超音波出力開始（測定開始）
#define trigPin_1  3 // Trigger Pin@ sensor 1
#define echoPin_2  4 // Echo Pin@ sensor 2
#define trigPin_2  5 // Trigger Pin@ sensor 2
#define echoPin_3  6 // Echo Pin@ sensor 3
#define trigPin_3  7 // Trigger Pin@ sensor 3
#define echoPin_4  8 // Echo Pin@ sensor 4
#define trigPin_4  9 // Trigger Pin@ sensor 4
#define tonePin  13  //output pin for speaker
#define ledPin1  A1 //output pin for led  //ピン指定名称正しいか要注意
#define ledPin2  A2 //output pin for led
#define ledPin3  A3 //output pin for led
#define ledPin4  A4 //output pin for led
#define ledPin5  A5 //output pin for led

#define sensor_1 1
#define sensor_2 2
#define sensor_3 3
#define sensor_4 4  

#define left_dst_thd  100  //左境界センサのブザー出力閾値（この値以下で警告
#define right_dst_thd  100  //右境界センサのブザー出力閾値（この値以下で警告
#define upper_left_dst_thd  130  //左上境界センサのブザー出力閾値（この値以下で警告
#define upper_right_dst_thd  130  //右上境界センサのブザー出力閾値（この値以下で警告

#define left_tone 262 //左境界センサ反応時ブザー音
#define right_tone 362 //右境界センサ反応時ブザー音
#define upper_left_tone 2620 //左上境界センサ反応時ブザー音
#define upper_right_tone 3620 //右上境界センサ反応時ブザー音
#define wait_tone 100 //

double Duration_1 = 0; //応答時間＠ sensor 1
double Distance_1 = 0; //距離@ sensor 1
double Duration_2 = 0; //応答時間＠ sensor 2
double Distance_2 = 0; //距離@ sensor 2
double Duration_3 = 0; //応答時間＠ sensor 3
double Distance_3 = 0; //距離@ sensor 3
double Duration_4 = 0; //応答時間＠ sensor 4
double Distance_4 = 0; //距離@ sensor 4
char incomingByte = 0;  //シリアル入力用変数

void setup()
{
  Serial.begin( 9600 );
  pinMode(echoPin_1, INPUT);
  pinMode(trigPin_1, OUTPUT);
  pinMode(echoPin_2, INPUT);
  pinMode(trigPin_2, OUTPUT);
  pinMode(echoPin_3, INPUT);
  pinMode(trigPin_3, OUTPUT);
  pinMode(echoPin_4, INPUT);
  pinMode(trigPin_4, OUTPUT);

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);

//  MCP mcp23s17(0,0);  //ioexpander 16bit(adress, ?)
}
void loop()
{
//キャリブレーションモード 通常モード
//起動時5秒間　ブザーなりっぱなし＆LED全灯になるのでその間にタクトSWを押すとキャリブレーションモードに入る
  for(int i; i<1000; i++)
  {
    if(Serial.available())
    {
      incomingByte = Serial.read();
      if(incomingByte == '1') sensor_calibration();  //
    }
    digitalWrite(ledPin1,HIGH);
    digitalWrite(ledPin2,HIGH);
    digitalWrite(ledPin3,HIGH);
    digitalWrite(ledPin4,HIGH);
    tone(tonePin, 100);
    delay(5);
  }
  noTone(tonePin);
  digitalWrite(ledPin1,LOW);
  digitalWrite(ledPin2,LOW);
  digitalWrite(ledPin3,LOW);
  digitalWrite(ledPin4,LOW);
  

  
//超音波出力＆反射時間取得 
  Duration_1 = start_measure(sensor_1); 
  Duration_2 = start_measure(sensor_2);
  Duration_3 = start_measure(sensor_3);
  Duration_4 = start_measure(sensor_4);

//時間を距離に変換(シリアル通信で出力しないなら不要→処理高速化）
  Distance_1 = dir_to_dis(Duration_1);  
  Distance_2 = dir_to_dis(Duration_2);
  Distance_3 = dir_to_dis(Duration_3);
  Distance_4 = dir_to_dis(Duration_4);

//シリアル通信で距離を出力（シリアル通信で出力しないなら不要→処理高速化）
  print_Distance(sensor_1); 
  print_Distance(sensor_2);
  print_Distance(sensor_3);
  print_Distance(sensor_4); 
  Serial.print("\n");

//センサが何かを補足したらブザー
  if(Distance_1 < left_dst_thd) 
  {
    tone(tonePin, left_tone, wait_tone);
    delay(wait_tone);
  }
  if(Distance_2 < upper_left_dst_thd) 
  {
    tone(tonePin, upper_left_tone, wait_tone);
    delay(wait_tone);
  }
  if(Distance_3 < upper_right_dst_thd) 
  {
    tone(tonePin, upper_right_tone, wait_tone);
    delay(wait_tone);
  }
  if(Distance_4 < right_dst_thd) 
  {
    tone(tonePin, right_tone, wait_tone);
    delay(wait_tone);
  }
}

//*****関数群*****//

//センサごとの測定距離をPCにシリアル出力
void print_Distance(int sensor_number)
{
  switch(sensor_number)
  {
    case sensor_1:
     if(Distance_1 == 0)
     {
        Serial.print("Sensor_1 time out");
        Serial.print("\n");
     }
     else if (Distance_1 > 0)
     {
        Serial.print("Sensor_1  :  ");
        Serial.print(Distance_1);
        Serial.println(" cm");
     }
     else Serial.print("Error_sensor_1");
     break;

    case sensor_2:
     if(Distance_2 == 0)
     {
        Serial.print("Sensor_2 time out");
        Serial.print("\n");
     }
     else if (Distance_2 > 0)
     {
        Serial.print("Sensor_2  :  ");
        Serial.print(Distance_2);
        Serial.println(" cm");
     }
     else Serial.print("Error_sensor_2");
     break;

    case sensor_3:
     if(Distance_3 == 0)
     {
        Serial.print("Sensor_3 time out");
        Serial.print("\n");
     }
     else if (Distance_3 > 0)
     {
        Serial.print("Sensor_3  :  ");
        Serial.print(Distance_3);
        Serial.println(" cm");
     }
     else Serial.print("Error_sensor_3");
     break;

     case sensor_4:
     if(Distance_4 == 0)
     {
        Serial.print("Sensor_4 time out");
        Serial.print("\n");
     }
     else if (Distance_4 > 0)
     {
        Serial.print("Sensor_4  :  ");
        Serial.print(Distance_4);
        Serial.println(" cm");
     }
     else Serial.print("Error_sensor_4");
     break;

     default:
      break;
  }
//  Serial.print("\n");
  delay(250);
}

//指定したセンサの計測を開始＆測定値（時間）を返す
double start_measure(int sensor_number)
{
  double Duration = 0;
  
  switch(sensor_number)
  {
    case sensor_1:  
      digitalWrite(trigPin_1, LOW);  
      digitalWrite( trigPin_1, HIGH );
      delayMicroseconds( 10 ); //最小信号時間10us 超音波出力信号
      digitalWrite( trigPin_1, LOW );
      Duration = pulseIn( echoPin_1, HIGH, 20000); //センサからの入力(注意：タイムアウト時間/2が片道時間 //20000=20ms 片道時間10msで3.4m計測可能  
      break;
    case sensor_2:
     digitalWrite(trigPin_2, LOW);
      digitalWrite( trigPin_2, HIGH );
      delayMicroseconds( 10 ); //最小信号時間10us 超音波出力信号
      digitalWrite( trigPin_2, LOW );
      Duration = pulseIn( echoPin_2, HIGH, 20000); //センサからの入力(注意：タイムアウト時間/2が片道時間 //20000=20ms 片道時間10msで3.4m計測可能  
      break;
    case sensor_3:
      digitalWrite(trigPin_3, LOW);
      digitalWrite( trigPin_3, HIGH );
      delayMicroseconds( 10 ); //最小信号時間10us 超音波出力信号
      digitalWrite( trigPin_3, LOW );
      Duration = pulseIn( echoPin_3, HIGH, 20000); //センサからの入力(注意：タイムアウト時間/2が片道時間 //20000=20ms 片道時間10msで3.4m計測可能  
      break;
    case sensor_4:
      digitalWrite(trigPin_4, LOW);
      digitalWrite( trigPin_4, HIGH );
      delayMicroseconds( 10 ); //最小信号時間10us 超音波出力信号
      digitalWrite( trigPin_4, LOW );
      Duration = pulseIn( echoPin_4, HIGH, 20000); //センサからの入力(注意：タイムアウト時間/2が片道時間 //20000=20ms 片道時間10msで3.4m計測可能  
      break;
    default:
      Serial.print("irregular sensor number");
      break;
  }
  return Duration;
}

//超音波センサの測定時間を測定距離に変換
double dir_to_dis(double Duration)
{
  double Distance;
  Duration = Duration/2; //往復時間を片道時間に
  Distance = Duration*340*100/1000000; //経過時間(us)* 音速(340m/s)*(m→cm)/(sec→usec)
  return Distance;
}

void sensor_calibration()
{
  //100回値を入力して尤度の高い値を中央値に置く（あまりに中央値から外れた値は無視して平均を算出して校正値とし、EEPROMに書き込む）
  
  int Distance[100];
  double Duration;
  long DistanceSum = 0;
  int DistanceAve;

  for(int i=0; i<100; i++)
  {
    Duration = start_measure(sensor_1); 
    Distance[i] = (int)dir_to_dis(Duration_1);  
    DistanceSum = Distance[i];
  }
  //ここで余りに外れた値をはじく
  //平均値を算出してEEPROMに書き込む
  DistanceAve = DistanceSum/100;
  
}

