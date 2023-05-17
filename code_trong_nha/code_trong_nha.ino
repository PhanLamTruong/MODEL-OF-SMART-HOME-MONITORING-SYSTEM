#include <Wire.h>
#include <WiFi.h>
#include "FirebaseESP32.h"
#include <ArduinoJson.h>
#include "DHT.h"

//sender phone number with country code
//const String PHONE = "+84358529005";

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN    14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define WIFI_SSID "Tuanhan"
#define WIFI_PASSWORD "1234567890"

//#define WIFI_SSID "NHA TRO HANH PHUC 1B"
//#define WIFI_PASSWORD "11111111B"

//#define WIFI_SSID "TP-Link_7D9C"
//#define WIFI_PASSWORD "khongbiet602"

#define FIREBASE_HOST "https://testdoantn-7a453-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "5qmIff1D7URWhiTzblQ5lVMKSDU1OlKNenLPwPW5"
FirebaseData firebaseData;
String path="/";
FirebaseJson json;

//GSM Module RX pin to ESP32 Pin 13
//GSM Module TX pin to ESP32 Pin 4
#define rxPin 4
#define txPin 13
#define BAUD_RATE 9600
HardwareSerial sim800(1);

#define RELAY_1 17
#define RELAY_2 15
#define RELAY_3 16
#define RELAY_4 18


//int LM35_Raw_Sensor1 = 0;
//float LM35_TempC_Sensor1 = 0.0;
//float LM35_TempF_Sensor1 = 0.0;
//float Voltage = 0.0;
//float t =0.0;
//float h =0.0;

int Buzzer = 32;       // used for ESP32
int Gas_analog = 33;    // used for ESP32

int nutnhan1=25;
int nutnhan2=26;

  int Mode = 1;
  int dem1=1;
  int dem2=1;
  int dem3=1;
  int dem4=1;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Power
#define BH1750_POWER_DOWN 0x00  // No active state
#define BH1750_POWER_ON 0x01  // Waiting for measurement command
#define BH1750_RESET 0x07  // Reset data register value - not accepted in POWER_DOWN mode

// Measurement Mode
#define CONTINUOUS_HIGH_RES_MODE 0x10  // Measurement at 1 lux resolution. Measurement time is approx 120ms
#define CONTINUOUS_HIGH_RES_MODE_2 0x11  // Measurement at 0.5 lux resolution. Measurement time is approx 120ms
#define CONTINUOUS_LOW_RES_MODE 0x13  // Measurement at 4 lux resolution. Measurement time is approx 16ms
#define ONE_TIME_HIGH_RES_MODE 0x20  // Measurement at 1 lux resolution. Measurement time is approx 120ms
#define ONE_TIME_HIGH_RES_MODE_2 0x21  // Measurement at 0.5 lux resolution. Measurement time is approx 120ms
#define ONE_TIME_LOW_RES_MODE 0x23  // Measurement at 4 lux resolution. Measurement time is approx 16ms

// I2C Address
#define BH1750_1_ADDRESS 0x23  // Sensor 1 connected to GND
#define BH1750_2_ADDRESS 0x5C  // Sensor 2 connected to VCC


String smsStatus,senderNumber,receivedDate,msg;
boolean isReply = false;

String PHONE;

int16_t RawData;
int16_t SensorValue[2];



void setup() {
  lcd.init();
  lcd.backlight();
  dht.begin();
  Wire.begin();

  pinMode(Buzzer, OUTPUT);

  pinMode(Gas_analog, INPUT);
    
  pinMode(RELAY_1, OUTPUT); //Relay 1
  pinMode(RELAY_2, OUTPUT); //Relay 2
  pinMode(RELAY_3, OUTPUT); //Relay 3
  pinMode(RELAY_4, OUTPUT); //Relay 4

  pinMode(nutnhan1,INPUT_PULLUP);
  pinMode(nutnhan2,INPUT_PULLUP);

  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(RELAY_3, LOW);
  digitalWrite(RELAY_4, LOW);
  
  
  Serial.begin(115200);
  Serial.println("esp32 serial initialize");
  
  sim800.begin(BAUD_RATE, SERIAL_8N1, rxPin, txPin);
  Serial.println("SIM800L serial initialize");
  
  smsStatus = "";
  senderNumber="";
  receivedDate="";
  msg="";

  sim800.print("AT+CMGF=1\r"); //SMS text mode
  delay(1000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print ("Connecting");
    delay(500);
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if (!Firebase.beginStream(firebaseData, path))
   {
     Serial.println("REASON: " + firebaseData.errorReason());
     Serial.println();
   }
  Serial.print("connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
     lcd.setCursor(0, 1);
    lcd.print ("Connected        ");
    delay (500);
    lcd.clear();
}

void loop() {
//////////////////////////////////////////////////
while(sim800.available()){
  parseData(sim800.readString());
}
//////////////////////////////////////////////////
while(Serial.available())  {
  sim800.println(Serial.readString());
}
//////////////////////////////////////////////////

if (Firebase.getString(firebaseData, path + "/Phone"))  PHONE = firebaseData.stringData();
  nhietgas();
  SendFirebase();
  Nutnhan();
  Websever(); 
 
//////////////////////////////////////////
} //main loop ends

//**************************************************
void Read2Sensor(){
  init_BH1750(BH1750_1_ADDRESS, CONTINUOUS_HIGH_RES_MODE);
  delay(120);
  RawData_BH1750(BH1750_1_ADDRESS);
  SensorValue[0] = RawData / 1.2;  

  init_BH1750(BH1750_2_ADDRESS, CONTINUOUS_HIGH_RES_MODE);
  delay(120);
  RawData_BH1750(BH1750_2_ADDRESS);
  SensorValue[1] = RawData / 1.2;

//  Serial.print("Sensor_1 = "); Serial.print(SensorValue[0]);
//  Serial.print(" | Sensor_2 = "); Serial.println(SensorValue[1]);
//  blinkState = !blinkState;
//  digitalWrite(LED_PIN, blinkState);
}

void init_BH1750(int ADDRESS, int MODE){
  //BH1750 Initializing & Reset
  Wire.beginTransmission(ADDRESS);
  Wire.write(MODE);  // PWR_MGMT_1 register
  Wire.endTransmission(true);
}

void RawData_BH1750(int ADDRESS){
  Wire.beginTransmission(ADDRESS);
  Wire.requestFrom(ADDRESS,2,true);  // request a total of 2 registers
  RawData = Wire.read() << 8 | Wire.read();  // Read Raw Data of BH1750
  Wire.endTransmission(true);
}
//***************************************************
  

//***************************************************
void parseData(String buff){
  Serial.println(buff);

  unsigned int len, index;
  //////////////////////////////////////////////////
  //Remove sent "AT Command" from the response string.
  index = buff.indexOf("\r");
  buff.remove(0, index+2);
  buff.trim();
  //////////////////////////////////////////////////
  
  //////////////////////////////////////////////////
  if(buff != "OK"){
    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();
    
    buff.remove(0, index+2);
    
    if(cmd == "+CMTI"){
      //get newly arrived memory location and store it in temp
      index = buff.indexOf(",");
      String temp = buff.substring(index+1, buff.length()); 
      temp = "AT+CMGR=" + temp + "\r"; 
      //get the message stored at memory location "temp"
      sim800.println(temp); 
    }
    else if(cmd == "+CMGR"){
      extractSms(buff);
//      Serial.println (senderNumber);
//      Serial.println (PHONE);
      
      if(senderNumber == PHONE){
        doAction();
      }
      else{
        if(msg == "change32"){  ////đổi số đt
        PHONE = senderNumber;
       Serial.print("doi so dien thoai: ");
       Serial.println(PHONE);
       Reply1("You changed numberphone");
       Firebase.setString(firebaseData, path + "/Phone", PHONE);  
      }
      else
        Reply1("You can't control");
        } 
    }
      
    
  ////////////////////////////////////////////////
  }
  else{
  //The result of AT Command is "OK"
  }
}

//************************************************************
void extractSms(String buff){
   unsigned int index;
   
    index = buff.indexOf(",");
    smsStatus = buff.substring(1, index-1); 
    buff.remove(0, index+2);
    
    senderNumber = buff.substring(0, 12);
    buff.remove(0,19);
   
    receivedDate = buff.substring(0, 20);
    buff.remove(0,buff.indexOf("\r"));
    buff.trim();
    
    index =buff.indexOf("\n\r");
    buff = buff.substring(0, index);
    buff.trim();
    msg = buff;
    buff = "";
    msg.toLowerCase();
}

void doAction(){
  if(msg == "den1 off"){  ////relay1 off
    Serial.println("den 1 off");  
    //digitalWrite(RELAY_1, LOW);
    dem1=1;
    Firebase.setInt(firebaseData, path + "/Den1 ", dem1);
    Reply("Den 1 has been OFF");
  }
  else if(msg == "den1 on"){/////relay1 on
    Serial.println("den 1 on");
    //digitalWrite(RELAY_1, HIGH);
    dem1=2;
    Firebase.setInt(firebaseData, path + "/Den1 ", dem1);
    Reply("Den 1 has been ON");
  }
  else if(msg == "den2 off"){/////relay2 off

    Serial.println("den 2 off");
    //digitalWrite(RELAY_2, LOW);
    dem2=1;
    Firebase.setInt(firebaseData, path + "/Den2 ", dem2);
    Reply("Den 2 has been OFF");

  }
  else if(msg == "den2 on"){////relay2 on
    Serial.println("den 2 on");
    //digitalWrite(RELAY_2, HIGH);
    dem2=2;
    Firebase.setInt(firebaseData, path + "/Den2 ", dem2);
    Reply("Den 2 has been ON");
  }

  else if(msg == "quat1 on"){////relay3 on
    Serial.println("quat 1 on");
    //digitalWrite(RELAY_3, HIGH);
    dem3=2;
    Firebase.setInt(firebaseData, path + "/Quat1 ", dem3);
    Reply("Quat 1 has been ON");
  }
   else if(msg == "quat2 on"){////relay4 on
    Serial.println("quat 2 on");
    //digitalWrite(RELAY_3, HIGH);
    dem4=2;
    Firebase.setInt(firebaseData, path + "/Quat2 ", dem4);
    Reply("Quat 2 has been ON");
  }
   else if(msg == "quat1 off"){////relay3 oN
    Serial.println("quat 1 off");
    //digitalWrite(RELAY_3, HIGH);
    dem3=1;
    Firebase.setInt(firebaseData, path + "/Quat1 ", dem3);
    Reply("Quat 1 has been OFF");
  }

  else if(msg == "quat2 off"){////relay4 OFF
    Serial.println("quat 2 off");
    //digitalWrite(RELAY_4, HIGH);
    dem3=1;
    Firebase.setInt(firebaseData, path + "/Quat2 ", dem4);
    Reply("Quat 2 has been OFF");
  }
  
  else if(msg == "kt sdt"){////kiểm tra có phải là số đt có quyền điều khiển hay không
    Serial.println("kiem tra so dien thoai");
    Reply("You can control");
  }
  else if(msg == "kt den1"){////kiểm tra đèn 1
    Serial.println("Kiem tra den 1");
    Read2Sensor();
    if(SensorValue[0] > 1000){
    Reply("Den 1 has being ON");}
    else 
    Reply("Den 1 has being OFF");
  }
    else if(msg == "kt den2"){////kiểm tra đèn 1
    Serial.println("Kiem tra den 2");\
    Read2Sensor();
    if(SensorValue[1] > 1000){
    Reply("Den 2 has being ON");}
    else 
    Reply("Den 2 has being OFF");
  }
//    else if(msg == "kt nhiet do"){////kiểm tra nhiệt độ
//    Serial.println("Kiem tra nhiet do");
//    float t = dht.readTemperature();
//    Reply2("T: ", t);
//    }
//    else if(msg == "kt do am"){////kiểm tra nhiệt độ
//    Serial.println("Kiem tra nhiet do");
//    float h = dht.readHumidity();
//    Reply2("H: ", h);
//    }

  smsStatus = "";
  senderNumber="";
  receivedDate="";
  msg="";  
}

void Reply(String text)
{
    sim800.print("AT+CMGF=1\r");
    delay(1000);
    sim800.print("AT+CMGS=\""+PHONE+"\"\r");
    delay(1000);
    sim800.print(text);
    delay(100);
    sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
    delay(1000);
    Serial.println("SMS Sent Successfully.");
}

void Reply1(String text)
{
    sim800.print("AT+CMGF=1\r");
    delay(1000);
    sim800.print("AT+CMGS=\""+senderNumber+"\"\r");
    delay(1000);
    sim800.print(text);
    delay(100);
    sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
    delay(1000);
    Serial.println("SMS Sent Successfully.");
}

void Reply2(String text, float a)
{
    sim800.print("AT+CMGF=1\r");
    delay(1000);
    sim800.print("AT+CMGS=\""+PHONE+"\"\r");
    delay(1000);
    sim800.print(text);
    sim800.print(a);
    delay(100);
    sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
    delay(1000);
    Serial.println("SMS Sent Successfully.");
}


//void Reply3(String text, int a , String text1, int b)
//{
//    sim800.print("AT+CMGF=1\r");
//    delay(1000);
//    sim800.print("AT+CMGS=\""+PHONE+"\"\r");
//    delay(1000);
//    sim800.print(text);
//    sim800.print(a);
//    sim800.print(text1);
//    sim800.print(b);
//    delay(100);
//    sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
//    delay(1000);
//    Serial.println("SMS Sent Successfully.");
//}
////*************************************
void nhietgas()
{

float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
//
  int gassensorAnalog = analogRead(Gas_analog);
  Serial. println (gassensorAnalog);
 // int gassensorAnalog = digitalRead(Gas_analog);
  
  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(t);

  lcd.setCursor(9, 1);
  lcd.print("H: ");
  lcd.print(h);

  lcd.setCursor(1, 0);
  lcd.print("Khi Gas: ");
  if (gassensorAnalog > 2000)
  {
    dem4 =2;
    Firebase.setInt(firebaseData, path + "/Quat2 ", dem4);
  lcd.setCursor(10, 0);  
  lcd.print(gassensorAnalog  );}
  else
 { 
  dem4 =1;
  Firebase.setInt(firebaseData, path + "/Quat2 ", dem4);
  lcd.setCursor(10, 0);  
  lcd.print(gassensorAnalog  );}
  
  if (gassensorAnalog > 2000) {
    if (Mode == 0){
    Reply2("Warning Gas: ", gassensorAnalog);
    Serial.println("Ro ri Gas");
    digitalWrite (Buzzer, HIGH) ; //send tone
    delay(1000);
    digitalWrite (Buzzer, LOW) ;  //no tone
    Mode = 1;
  }
    else {
    
    Serial.println("Ro ri Gas");
    digitalWrite (Buzzer, HIGH) ; //send tone
    delay(1000);
    digitalWrite (Buzzer, LOW) ;  //no tone
  
    }
  }
  else {
    Mode = 0;
   lcd.setCursor(10, 0);  
  lcd.print(gassensorAnalog  );
  lcd.print("  ");
//    Serial.println("No Gas");
  }
  Firebase.setInt(firebaseData, path + "/NHIET DO", t);
  Firebase.setInt(firebaseData, path + "/DO AM", h);
  Firebase.setInt(firebaseData, path + "/KHI GAS", gassensorAnalog);

  }


////**************************************
void SendFirebase(){

  Read2Sensor();
   Firebase.setInt(firebaseData, path + "/CDAS 1 ", SensorValue[0]);
   Firebase.setInt(firebaseData, path + "/CDAS 2 ", SensorValue[1]);
  }

void Nutnhan(){

  int trangthainut1=digitalRead(nutnhan1);
  int trangthainut2=digitalRead(nutnhan2);
        if (trangthainut1 ==0)
        {

          dem1 = dem1 + 1;
            if(dem1 > 2){
                dem1 =1;
       }       
        Firebase.setInt(firebaseData, path + "/Den1 ", dem1);  
        }

        if (trangthainut2 ==0)
        {

          dem2 = dem2 + 1;
            if(dem2 > 2){
                dem2 =1;
       }       
         Firebase.setInt(firebaseData, path + "/Den2 ", dem2); 
        }

        if (dem1==1)
        digitalWrite(RELAY_1, LOW);
        else
        digitalWrite(RELAY_1, HIGH);
        if (dem2==1)
        digitalWrite(RELAY_2, LOW);
        else
        digitalWrite(RELAY_2, HIGH);

        if (dem3==1)
        digitalWrite(RELAY_3, LOW);
        else
        digitalWrite(RELAY_3, HIGH);
        if (dem4==1)
        digitalWrite(RELAY_4, LOW);
        else
        digitalWrite(RELAY_4, HIGH);
        
}
void Websever()
{
  if (Firebase.getInt(firebaseData, path + "/Den1 "))  dem1 = firebaseData.intData();
  if (Firebase.getInt(firebaseData, path + "/Den2 "))  dem2 = firebaseData.intData();
  if (Firebase.getInt(firebaseData, path + "/Quat1 "))  dem3 = firebaseData.intData();
  if (Firebase.getInt(firebaseData, path + "/Quat2 "))  dem4 = firebaseData.intData();

        if (dem1==1)
        digitalWrite(RELAY_1, LOW);
        else
        digitalWrite(RELAY_1, HIGH);
        if (dem2==1)
        digitalWrite(RELAY_2, LOW);
        else
        digitalWrite(RELAY_2, HIGH);

        if (dem3==1)
        digitalWrite(RELAY_3, LOW);
        else
        digitalWrite(RELAY_3, HIGH);
        if (dem4==1)
        digitalWrite(RELAY_4, LOW);
        else
        digitalWrite(RELAY_4, HIGH);
  
  }



  
