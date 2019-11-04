//峥果触摸开关面板
//Esp8266 Core: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//EspMQTTClient:  https://github.com/plapointe6/EspMQTTClient
/*
 * 特别说明：需要按照以下库文件版本才可编译通过
 * 依赖库文件：
 *          tasker 2.0 
 *          EspMQTTClient 1.6.2
 *          PubSubClient 2.7
 *          ESP8266开发板 2.4.2
 */

/*
 * 20191103 修复MQTT控制与当前的按钮开关不一致问题，MQTTCallback增加了对MQTT的payload的信息判断
 * 20191103 新增 MQTT设备名配置（支持多设备）
 * 20191103 新增 单双电机配置 
 * 20191103 新增 新增联动配置
 * 20191103 新增 新增风铃开关配置
 */
#include "Tasker.h"
#include "EspMQTTClient.h"
Tasker SchTask;

/*
 * MQTT库参数设置,根据实际情况进行修改
 */
EspMQTTClient client(
  "ChinaNet-87230612",                // Wifi ssid
  "eeekvj9q",            // Wifi password
  "192.168.1.165",             // MQTT服务器IP地址
  "mqtt",                // MQTT服务器用户名
  "mqtt",            // MQTT服务器密码  
  "ZGYB",                     // 客户端名称ZGYB
  1883                        // MQTT服务器端口
);

const char*  DEV_SET[] = {
  "YB2", //设备名：多个设备自行更改
  "0", //单双电机配置：0:单电机|1:双电机
  "1", //暖1暖2 联动吹风：0:关闭|1:暖1暖2开启吹风联动|2:暖1开启吹风联动|3:暖2开启吹风联动
  "0", //是否开启按键声音提示：0:关闭|1:开启
};

//MQTT主题列表
//key1:照明,Q //key2: 换气,Q //key3:全关 //key6:取暖2,R //key7:吹风,Q //key8:取暖1,R
const char* MQTT_Pub[] = {
  "/1", //key1:照明,Q
  "/2", //key2:换气,Q
  "/3", //key3:全关
  "/6", //key6:取暖2,R
  "/7", //key7:吹风,Q
  "/8", //key8:取暖1,R
  "/temp"
};

const char* MQTT_Sub[] = {
  "/1/set", //key1:照明,Q
  "/2/set", //key2:换气,Q
  "/3/set", //key3:全关
  "/6/set", //key6:取暖2,R
  "/7/set", //key7:吹风,Q
  "/8/set", //key8:取暖1,R
};


#define twi_sda 4                                   //SDA引脚IO定义,for sc09a
#define twi_scl 5                                   //SCL引脚IO定义,for sc09a
#define SDA_LOW()   (GPES = (1 << twi_sda))         //SDA置LOW,for sc09a
#define SDA_HIGH()  (GPEC = (1 << twi_sda))         //SDA置HIGH,for sc09a
#define SCL_LOW()   (GPES = (1 << twi_scl))         //SCL置LOW,for sc09a
#define SCL_HIGH()  (GPEC = (1 << twi_scl))         //SCL置HIGH,for sc09a
#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)   //SDA读内容,for sc09a
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)   //SCL读内容,for sc09a
#define CON_ADDR 0x81                               //0x40(器件地址)+0x01(读取)=0x81,for sc09a

#define ON_K1  CtrlBuff[2]&=~(1<<2);CtrlBuff[3]|=(1<<0);CtrlBuff[4]|=(1<<0)   //TocuhKey1开启
#define OF_K1  CtrlBuff[2]|=(1<<2);CtrlBuff[3]&=~(1<<0);CtrlBuff[4]&=~(1<<0)  //TocuhKey1关闭
#define ON_K2  CtrlBuff[2]&=~(1<<4);CtrlBuff[3]|=(1<<1);CtrlBuff[4]|=(1<<1)   //TocuhKey2开启
#define OF_K2  CtrlBuff[2]|=(1<<4);CtrlBuff[3]&=~(1<<1);CtrlBuff[4]&=~(1<<1)  //TocuhKey2关闭
#define ON_K6  CtrlBuff[2]|=(1<<6);CtrlBuff[3]|=(1<<5);CtrlBuff[4]|=(1<<5)    //TocuhKey6开启
#define OF_K6  CtrlBuff[2]&=~(1<<6);CtrlBuff[3]&=~(1<<5);CtrlBuff[4]&=~(1<<5) //TocuhKey6关闭
#define ON_K7  CtrlBuff[2]&=~(1<<3);CtrlBuff[3]|=(1<<4);CtrlBuff[4]|=(1<<6)   //TocuhKey7开启
#define OF_K7  CtrlBuff[2]|=(1<<3);CtrlBuff[3]&=~(1<<4);CtrlBuff[4]&=~(1<<6)  //TocuhKey7关闭
#define ON_K8  CtrlBuff[2]|=(1<<5);CtrlBuff[3]|=(1<<3);CtrlBuff[4]|=(1<<7)    //TocuhKey8开启
#define OF_K8  CtrlBuff[2]&=~(1<<5);CtrlBuff[3]&=~(1<<3);CtrlBuff[4]&=~(1<<7) //TocuhKey8关闭

const unsigned char dataPin = 14;                   //74HC595数据:旧GPIO13，新GPIO14
const unsigned char loadPin = 12;                   //74HC595锁存：旧GPIO14，新GPIO12
const unsigned char clockPin = 13;                  //74HC595时钟：旧GPIO0，新GPIO13
const unsigned char Beep =  9;                      //风铃器Beep

//数码管0~9,Fix For New
const unsigned char DigitNUM[] = {0x81, 0x9F, 0xA2, 0x92, 0x9C, 0xD0, 0xC0, 0x9B, 0x80, 0x90};

//控制缓冲区：0温度，1按键，2输出端子，3LED缓冲，4输出缓冲
unsigned char CtrlBuff[] = {0x00, 0x00, 0x1c, 0x00, 0x00};

boolean antishake = false;                          //按键功能锁闭，防抖动
unsigned short TouchKey = 0x0000;                   //定义2个字节的Key缓存

//定义MQTT主题
String  MQTT_ZT =  "ZINGUO/" + String(DEV_SET[0]);

//MQTT订阅回调函数
void onConnectionEstablished()
{
  client.subscribe(MQTT_ZT + MQTT_Sub[0], [] (const String & payload)
  {
    MQTTCallback(1, 1, payload); //消息处理回调：接收模式，键值1，消息内容
  });
  
  client.subscribe(MQTT_ZT + MQTT_Sub[1], [] (const String & payload)
  {
    MQTTCallback(1, 2, payload); //消息处理回调：接收模式，键值2，消息内容
  });
  
  client.subscribe(MQTT_ZT + MQTT_Sub[2], [] (const String & payload)
  {
    MQTTCallback(1, 3, payload); //消息处理回调：接收模式，键值3，消息内容
  });
  
  client.subscribe(MQTT_ZT + MQTT_Sub[3], [] (const String & payload)
  {
    MQTTCallback(1, 6, payload); //消息处理回调：接收模式，键值6，消息内容
  });
  
  client.subscribe(MQTT_ZT + MQTT_Sub[4], [] (const String & payload)
  {
    MQTTCallback(1, 7, payload); //消息处理回调：接收模式，键值7，消息内容
  });
  
  client.subscribe(MQTT_ZT + MQTT_Sub[5], [] (const String & payload)
  {
    MQTTCallback(1, 8, payload); //消息处理回调：接收模式，键值8，消息内容
  });
}

//MQTT数据处理程序
//MQTTCallBack(1,2,3)
//参数：1=发送0或接收1，2=主题数组编号[键值6/7/8/3/2/1]，3=消息内容[0/1]
void MQTTCallback(uint8_t RxTx, uint8_t KeyNum, const String PayLoad)
{
  if (0 == RxTx)
  {
    switch (KeyNum)
    {
      case 1: // 照明灯
      client.publish(MQTT_ZT + String(MQTT_Pub[0]), PayLoad);
      break;
      case 2: // 换气
      client.publish(MQTT_ZT + String(MQTT_Pub[1]), PayLoad);
      break;
      case 3: // 全关
      client.publish(MQTT_ZT + String(MQTT_Pub[2]), PayLoad);
      break;
      case 6: // 取暖2
      client.publish(MQTT_ZT + String(MQTT_Pub[3]), PayLoad);
      break;
      case 7: // 吹风
      client.publish(MQTT_ZT + String(MQTT_Pub[4]), PayLoad);
      break;
      case 8: // 取暖1
      client.publish(MQTT_ZT + String(MQTT_Pub[5]), PayLoad);
      break;
      case 0: // 温度
      client.publish(MQTT_ZT + String(MQTT_Pub[6]), PayLoad);
      break;
    }
  }
  else if (1 == RxTx)
  {
    CtrlBuff[1] = KeyNum;
    
    // MQTT控制开
    Serial.print("PayLoad:");
    Serial.println(PayLoad);
    if ((PayLoad == "1") and (bitRead(CtrlBuff[4], CtrlBuff[1] - 1) == 0))
    {
      Serial.println("exec on");
      AnalysisKey(CtrlBuff[1]);
    }

    // MQTT控制关
    if ((PayLoad == "0") and (bitRead(CtrlBuff[4], CtrlBuff[1] - 1) == 1))
    {
      Serial.println("exec off");
      AnalysisKey(CtrlBuff[1]);
    }  
  }
}


//ADC读取NTC温度转换程序
void ConvertTemp()                                         
{
  float hq = 3.50 * (analogRead(A0)) / 1023 ; //计算ADC采样值为电压值
  float x = 225 * hq / (3.50 - hq);           //计算当前的电阻值
  float hs = log(x / 150);                    //计算NTC对应阻值的对数值
  float temp = 1.0 / ( hs / 3453 + 1 / 298.15) - 273.15; //计算当前的温度值，可能的B值=3435、3450、3950、3980、4200
  CtrlBuff[0] = int(temp);                    //输出温度值
  
  MQTTCallback(0, 0, String(temp).c_str());
}

void setup()
{
  Serial.begin(115200);                      //设置串口波特率
  pinMode(dataPin, OUTPUT);                   //74HC595数据
  pinMode(loadPin, OUTPUT);                   //74HC595锁存
  pinMode(clockPin, OUTPUT);                  //74HC595时钟
  pinMode(Beep, OUTPUT);                      //风铃器引脚
  delay(1000);                                //延时1s,等待初始化,,for sc09a
  pinMode(twi_sda, INPUT_PULLUP);             //SDA,for sc09a
  pinMode(twi_scl, INPUT_PULLUP);             //SCL,for sc09a
  ConvertTemp();                              //初始化读取温度
  DispCtrl();                                 //初始化输出端口
  SchTask.setInterval(ConvertTemp, 5000);     //循环任务：每5s读取一次温度值
  //风铃器关闭 //循环任务：每5s读取一次温度值
  SchTask.setInterval([]() { Serial.println(CtrlBuff[2],BIN);}, 1000);

  client.enableHTTPWebUpdater(); //开启在线更新
  client.enableDebuggingMessages();//调试信息输出
}

void loop()
{
  unsigned short Key;                         //定义2个字节的Key缓存
  Key = GetKey();                             //获取键值
  if (TouchKey != Key)                        //如果前后按键不相等，则处理按键值
  {
    //—————————--———Debug————-——————————
    Serial.printf("TouchKey: 0x%0X", Key);
    Serial.println();
    //———————————————————————————————
    TouchKey = Key;                           //缓冲当前按键键值
    AnalysisKey(Key);                         //解析按键值
  }
  DispCtrl();                                 //刷新数码管、LED灯、74HC595
  SchTask.loop();                             //任务调度监控

  client.loop();                              //MQTT客户端循环线程
}

void BeepBeep(char i)                     //风铃器，参数为响声次数
{
  SchTask.setRepeated([]() {
    digitalWrite(Beep, HIGH);             //风铃器开启
    SchTask.setTimeout([]() {
      digitalWrite(Beep, LOW);            //风铃器关闭
    }, 70);
  }, 80, i);
}

void DispCtrl() //显示、控制数据的输出
{
  //控制缓冲区：0温度，1按键，2输出端子，3LED缓冲，4输出缓冲
  //第二个595先送数据，第一个595后送数据。
  //数码管十位，刷新数码管温度值
  byte The1st595 = B10000001 | CtrlBuff[2];                 //8Bit：控制蓝灯(0关/1开),7~3Bit:继电器可控硅，2~1Bit：数码管位
  byte The2nd595 = B10000000 | DigitNUM[CtrlBuff[0] / 10];  //8Bit：关闭红灯(1关/0开)，7~1Bit：数码管A-F(1关/0开)
  shiftOut(dataPin, clockPin, MSBFIRST, The2nd595);         //第二个595,
  shiftOut(dataPin, clockPin, MSBFIRST, The1st595);         //第一个595，控制继电器
  digitalWrite(loadPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(loadPin, LOW);

  //数码管个位，刷新数码管温度值
  The1st595 = B10000010 | CtrlBuff[2];                      //8Bit：控制蓝灯(0关/1开),7~3Bit:继电器可控硅，2~1Bit：数码管位
  The2nd595 = B10000000 | DigitNUM[CtrlBuff[0] % 10];       //8Bit：关闭红灯(1关/0开)，7~1Bit：数码管A-F(1关/0开)
  shiftOut(dataPin, clockPin, MSBFIRST, The2nd595);         //第二个595,
  shiftOut(dataPin, clockPin, MSBFIRST, The1st595);         //第一个595，控制继电器
  digitalWrite(loadPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(loadPin, LOW);

  //红色LED，待机状态
  The1st595 = B00000000 | CtrlBuff[2];                      //8Bit：控制红灯(1关/0开),7~3Bit:继电器可控硅，2~1Bit：数码管位
  The2nd595 = B10000000 | ~CtrlBuff[3];                     //8Bit：控制蓝灯(0关/1开)，7~1Bit：红色LED(1关/0开)
  shiftOut(dataPin, clockPin, MSBFIRST, The2nd595);         //第二个595,
  shiftOut(dataPin, clockPin, MSBFIRST, The1st595);         //第一个595，控制继电器
  digitalWrite(loadPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(loadPin, LOW);

  //蓝色LED，激活状态
  The1st595 = B10000000 | CtrlBuff[2];                      //8Bit：控制红灯(1关/0开),7~3Bit:继电器可控硅，2~1Bit：数码管位
  The2nd595 = B00000000 | CtrlBuff[3];                      //8Bit：控制蓝灯(0关/1开)，7~1Bit：蓝灯LED(1关/0开)
  shiftOut(dataPin, clockPin, MSBFIRST, The2nd595);         //第二个595,
  shiftOut(dataPin, clockPin, MSBFIRST, The1st595);         //第一个595，控制继电器
  digitalWrite(loadPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(loadPin, LOW);
}

void K2_ON()                                       //k2开
{
  if (DEV_SET[3] == "1"){BeepBeep(1);}
  CtrlBuff[3] |= (1 << 1);
  CtrlBuff[4] |= (1 << 1);
  if (bitRead(CtrlBuff[4], 6))                    //判断K7状态
  {
    OF_K7;
    MQTTCallback(0, 7, "0");
    CtrlBuff[2] &= ~(1 << 4);
    ON_K2;
    MQTTCallback(0, 2, "1");
  }
  else
  {
    CtrlBuff[2] &= ~(1 << 4);
    MQTTCallback(0, 2, "1");
  }
}
    
void AnalysisKey(unsigned short code)             //按键数据处理
{
  /*
    写1
    CtrlBuff[3] |= (1 << );
    CtrlBuff[4] |= (1 << );
    写0
    CtrlBuff[3] &= ~(1 << );
    CtrlBuff[4] &= ~(1 << );
    R为继电器，1开0关；Q=可控硅，1关0开
    输出【2】.RRQQQ..   .68271..0x1C=B0001 1100
    键灯【3】..111111   ..678321
    标志【4】111..111   876..321
  */
  switch (code)                                             //键值处理，对应键位
  {
    case 0x0400:                                            //key1:照明,Q
      CtrlBuff[1] = 1;
      break;
    case 0x0800:                                            //key2:排风,Q
      CtrlBuff[1] = 2;
      break;
    case 0x1000:                                            //key3:全关
      CtrlBuff[1] = 3;
      break;
    case 0x0100:                                            //key6:取暖2,R
      CtrlBuff[1] = 6;
      break;
    case 0x0200:                                            //key7:吹风,Q
      CtrlBuff[1] = 7;
      break;
    case 0x0080:                                            //key8:取暖1,R
      CtrlBuff[1] = 8;
      break;
  }
  if (code)                                                 //处理具体的按键事务
  {
    if (bitRead(CtrlBuff[4], CtrlBuff[1] - 1))
    {
      switch (CtrlBuff[1])//关
      {
        case 1:                                             //照明Q
          if (DEV_SET[3] == "1"){BeepBeep(1);}  
          OF_K1;
          MQTTCallback(0, 1, "0");
          break;
        case 2:                                             //换气Q
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          OF_K2;
          MQTTCallback(0, 2, "0");
          break;
        case 6:                                             //取暖2R
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          if (DEV_SET[2] == "1") //暖1暖2联动吹风延时关闭
          {
            if (!bitRead(CtrlBuff[4], 7))
            { //如果取暖1是关闭的，则延时关闭吹风
              SchTask.setTimeout([]() {                       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 7) && !bitRead(CtrlBuff[4], 5))
                {OF_K7;
                MQTTCallback(0, 7, "0");} 
                }, 15000);
            }
          }
          if (DEV_SET[2] == "3") //暖2联动吹风延时关闭
          {
              SchTask.setTimeout([]() {                       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 5))
                {OF_K7;
                MQTTCallback(0, 7, "0");} 
                }, 15000);
          }
          OF_K6;
          MQTTCallback(0, 6, "0");
          break;
        case 7:                                             //吹风
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          if (DEV_SET[2] == "0") //无吹风联动
          {
            OF_K7;
            MQTTCallback(0, 7, "0");
            }
          if (DEV_SET[2] == "1")//暖1暖2吹风联动
          {
            if (!bitRead(CtrlBuff[4], 7) && !bitRead(CtrlBuff[4], 5))
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
            else
            {
              BeepBeep(2);
            }
          }
          if (DEV_SET[2] == "2")//暖1吹风联动
          {
            if (!bitRead(CtrlBuff[4], 7))
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
            else
            {
              BeepBeep(2);
            }
          }
          if (DEV_SET[2] == "3")//暖2吹风联动
          {
            if (!bitRead(CtrlBuff[4], 5))
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
            else
            {
              BeepBeep(2);
            }
          }
          break;
        case 8:                                             //取暖1
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          if (DEV_SET[2] == "1") //暖1暖2联动吹风
          {         
            if (!bitRead(CtrlBuff[4], 5))
            { //如果取暖2是关闭的，则延时关闭吹风
              SchTask.setTimeout([]() {                       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 7) && !bitRead(CtrlBuff[4], 5))
                {
                  OF_K7;
                  MQTTCallback(0, 7, "0");
                }
              }, 15000);
            }
          }
          if (DEV_SET[2] == "2") //暖1联动吹风
          {         
            if (!bitRead(CtrlBuff[4], 5))
            { //如果取暖2是关闭的，则延时关闭吹风
              SchTask.setTimeout([]() {                       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 7))
                {
                  OF_K7;
                  MQTTCallback(0, 7, "0");
                }
              }, 15000);
            }
          }
          OF_K8;
          MQTTCallback(0, 8, "0");
          break;
      }
    }
    else
    {
      switch (CtrlBuff[1])//开
      {
        case 1:                                             //照明
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          ON_K1;                                            //开启照明
          MQTTCallback(0, 1, "1");
          break;
        case 2:                                             //吹风
          if (DEV_SET[1] == "0") //判断是否为单电机
          {
            if (DEV_SET[2] == "0")//无联动
            {
              K2_ON();
            }
            if (DEV_SET[2] == "1") //暖1暖2联动
            {
              if (!bitRead(CtrlBuff[4], 7) && !bitRead(CtrlBuff[4], 5))
              { //必须同时满足k6、k8都关闭，才能在k7开启的情况下切k2开启
                K2_ON();
              }
              else
              {
                BeepBeep(2);
              }
            }
            if (DEV_SET[2] == "2") //暖1联动
            {
              if (!bitRead(CtrlBuff[4], 7))
              { //满足k8关闭，才能在k7开启的情况下切k2开启
                K2_ON();
              }
              else
              {
                BeepBeep(2);
              }
            }
            if (DEV_SET[2] == "3") //暖2联动
            {
              if (!bitRead(CtrlBuff[4], 5))
              { //满足k6关闭，才能在k7开启的情况下切k2开启
                K2_ON();
              }
              else
              {
                BeepBeep(2);
              }
            }
          }
          else
          {
            CtrlBuff[3] |= (1 << 1);
            CtrlBuff[4] |= (1 << 1);
            CtrlBuff[2] &= ~(1 << 4);
            ON_K2;
            MQTTCallback(0, 2, "1");
          }

          break;
        case 6:                                       //取暖2        
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          if (DEV_SET[2] == "1" || DEV_SET[2] == "3") //暖2联动
          {
          if (DEV_SET[1] == "0") //单电机关闭换气后开启吹风
          {
            OF_K2;
            MQTTCallback(0, 2, "0");
          }
          ON_K7;
          MQTTCallback(0, 7, "1");
          ON_K6;
          MQTTCallback(0, 6, "1");
          }
          else 
          {
            ON_K6;
            MQTTCallback(0, 6, "1");
          }
          break;
        case 7:                                             //吹风
          if (DEV_SET[1] == "0") //判断是否为单电机
          {
            if (bitRead(CtrlBuff[4], 1))                    //判断K2状态
            {
              if (DEV_SET[3] == "1"){BeepBeep(1);}
              OF_K2;
              MQTTCallback(0, 2, "0");
              CtrlBuff[2] &= ~(1 << 3);
              ON_K7;
              MQTTCallback(0, 7, "1");
            }
            else
            {
              CtrlBuff[2] &= ~(1 << 3);
              MQTTCallback(0, 7, "1");
            }
            CtrlBuff[3] |= (1 << 4);
            CtrlBuff[4] |= (1 << 6);     
          }
          else
          {
            if (DEV_SET[3] == "1"){BeepBeep(1);}
            CtrlBuff[2] &= ~(1 << 3);
            ON_K7;
            MQTTCallback(0, 7, "1");
            CtrlBuff[3] |= (1 << 4);
            CtrlBuff[4] |= (1 << 6);
          }
          break;
        case 8:                                        //取暖1
          if (DEV_SET[3] == "1"){BeepBeep(1);}   
          if (DEV_SET[2] == "1" || DEV_SET[2] == "2") //暖1联动
          {
            if (DEV_SET[1] == "0") //单电机关闭换气后开启吹风
            {
              OF_K2;
              MQTTCallback(0, 2, "0");
            }
            ON_K7;
            MQTTCallback(0, 7, "1");
            ON_K8;
            MQTTCallback(0, 8, "1");
          }
          else
          {
            ON_K8;
            MQTTCallback(0, 8, "1");
          }
          break;
        default:                                     //3键全关操作
          if (DEV_SET[3] == "1"){BeepBeep(1);}
          if (DEV_SET[2] == "0") //无联动
          {
            OF_K7;
            MQTTCallback(0, 7, "0");
          }
          if (DEV_SET[2] == "1") //暖1暖2联动
          {
            if (bitRead(CtrlBuff[4], 7) || bitRead(CtrlBuff[4], 5))
            { //如果k6、k8任一处于开启状态，则执行延时关闭k7，否则立即关闭
              SchTask.setTimeout([]() {       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 7) && !bitRead(CtrlBuff[4], 5))
                { //延时时间到，如果k6、k8任一开启，则取消关闭k7
                  OF_K7;
                  MQTTCallback(0, 7, "0");
                }
              }, 15000);
            }
            else
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
          }
          if (DEV_SET[2] == "2") //暖1联动
          {
            if (bitRead(CtrlBuff[4], 7))
            { //如果k6、k8任一处于开启状态，则执行延时关闭k7，否则立即关闭
              SchTask.setTimeout([]() {       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 7))
                { //延时时间到，如果k6、k8任一开启，则取消关闭k7
                  OF_K7;
                  MQTTCallback(0, 7, "0");
                }
              }, 15000);
            }
            else
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
          }
          if (DEV_SET[2] == "3") //暖2联动
          {
            if (bitRead(CtrlBuff[4], 5))
            { //如果k6、k8任一处于开启状态，则执行延时关闭k7，否则立即关闭
              SchTask.setTimeout([]() {       //延时关闭吹风
                if (!bitRead(CtrlBuff[4], 5))
                { //延时时间到，如果k6、k8任一开启，则取消关闭k7
                  OF_K7;
                  MQTTCallback(0, 7, "0");
                }
              }, 15000);
            }
            else
            {
              OF_K7;
              MQTTCallback(0, 7, "0");
            }
          }
          OF_K1;
          MQTTCallback(0, 1, "0");
          OF_K2;
          MQTTCallback(0, 2, "0");
          OF_K6;
          MQTTCallback(0, 6, "0");
          OF_K8;
          MQTTCallback(0, 8, "0");
      }
    }
  }
}

static void ICACHE_RAM_ATTR twi_delay(unsigned char v)    //对Esp8266进行功能修正,for sc09a
{
  unsigned int i;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  unsigned int reg;
  for (i = 0; i < v; i++) {
    reg = GPI;
  }
  (void)reg;
#pragma GCC diagnostic pop
}

unsigned short GetKey(void)     //SC09A芯片iic精简协议模拟,for sc09a
{
  unsigned char bitnum, temp, addr;
  unsigned int key2byte;
  uint8_t bit_temp;
  addr = CON_ADDR;
  key2byte = 0xffff;
  SDA_LOW();// 拉低SDA 端口送出START 信号
  twi_delay(5);
  for (bitnum = 0; bitnum < 8; bitnum++)//发送8 位地址字节(A[6:0]+RWB)
  {
    SCL_LOW();
    temp = addr & 0x80;
    if (temp == 0x80)
      SDA_HIGH();
    else
      SDA_LOW();
    addr = addr << 1;
    twi_delay(5);
    SCL_HIGH();
    twi_delay(5);
  }
  SDA_HIGH(); //释放SDA 端口,将SDA 设置为输入端口
  SCL_LOW();
  twi_delay(5);
  SCL_HIGH();
  twi_delay(5);
  bit_temp = SDA_READ();
  if (bit_temp) //读ack 回应
    return 0;
  for (bitnum = 0; bitnum < 16; bitnum++)//读16 位按键数据字节(D[15:0])
  {
    SCL_LOW();
    twi_delay(5);
    SCL_HIGH();
    twi_delay(5);
    bit_temp = SDA_READ();
    if (bit_temp)
    {
      key2byte = key2byte << 1;
      key2byte = key2byte | 0x01;
    }
    else
    {
      key2byte = key2byte << 1;
    }
  }
  SCL_LOW();
  SDA_HIGH();
  twi_delay(5);
  SCL_HIGH();
  twi_delay(5);
  SCL_LOW();
  SDA_LOW(); //发送NACK 信号
  twi_delay(5);
  SCL_HIGH();
  twi_delay(5);
  SDA_HIGH(); //释放SDA 端口,将SDA 设置为输入端口
  key2byte = key2byte ^ 0xffff;//取反操作，便于阅读键值
  return (key2byte); //返回按键数据
}
