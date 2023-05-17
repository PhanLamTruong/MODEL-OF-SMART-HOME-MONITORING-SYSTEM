#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include<LiquidCrystal_I2C.h> //lcd header file
LiquidCrystal_I2C lcd(0x27, 16,2);
#include <Wire.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "RTClib.h"

#define MODEM_RX 16
#define MODEM_TX 17
#define mySerial Serial2 // use for ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//#define WIFI_SSID "NHA TRO HANH PHUC 1B"
//#define WIFI_PASSWORD "11111111B"

#define WIFI_SSID "Tuanhan"
#define WIFI_PASSWORD "1234567890"

#define FIREBASE_HOST "https://testdoantn-7a453-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "5qmIff1D7URWhiTzblQ5lVMKSDU1OlKNenLPwPW5"
FirebaseData firebaseData;
String path="/";
FirebaseJson json;

 
int relay = 4; //d4
int ij;
int cambien = 34; //Chân cảm biến nối chân số 5 Arduino
int giatri;

int den = 5;



const byte rows = 4; //số hàng
const byte columns = 4; //số cột

char key = 0;

char code[]={'1','2','3','4'}; //Passcode needed you can change it just keep the format "4 digits as char array"
char c[4];                     //Array to get the code from the user

//Định nghĩa các giá trị trả về
char keys[rows][columns] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};
 
byte rowPins[rows] = {32, 33, 25, 26}; //Cách nối chân với ESP32
byte columnPins[columns] = {27, 14 , 12, 13};
 
//cài đặt thư viện keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, columnPins, rows, columns);


RTC_DS1307 rtc;


uint8_t id;

void setup()  
{
  lcd.init();
  lcd.backlight();
  pinMode(relay, OUTPUT);
  pinMode(cambien, INPUT);

  pinMode(den, OUTPUT);
  digitalWrite(den, LOW);

 
  Serial.begin(9600);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
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
  
  
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    lcd.print("Found sensor");
    delay(500);
  } else {
    Serial.println("Did not find fingerprint sensor");
    lcd.print("Not found sensor");
    delay(500);
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  Serial.print("Sensor contains "); 
  Serial.print(finger.templateCount); 
  Serial.println(" templates");
  Serial.println("Waiting for valid finger...");
  lcd.clear();
  lcd.print("   **********   ");
  lcd.setCursor(2,1);
  lcd.print("System Start");
  delay(1000);
  lcd.clear();

  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    }
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

////////////////////////***VOID LOOP***/////////////////////////////////

void loop()                     
{
  realtime();
  getFingerprintIDez();
  door_way();
  web_add();
  web_del();              
  char temp = keypad.getKey();
  
    if(temp == 'A') // Them van tay
    {
       ij=0;
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Nhap Password");
       getPassword();
       if(ij==4)
       {                //If the passcode is correct we can start enrolling new finger template
       Enrolling();              //Enrolling function
       delay(2000);
       lcd.clear();
       }
       else
       {
        lcd.setCursor(0,0);
        lcd.print("Sai Password");
        delay(2000);
        lcd.clear();        
       }
    }
    
    if(temp == 'B') // Xoa van tay
    {
       ij=0;
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Nhap Password");
       getPassword();
       if(ij==4)
       {                //If the passcode is correct we can start enrolling new finger template
       DeleteFP();              //Enrolling function
       delay(2000);
       lcd.clear();
       }
       else
       {
        lcd.setCursor(0,0);
        lcd.print("Sai Password");
        delay(2000);
        lcd.clear();        
       }
    } 
                      
}

////////////////////////////////////////END VOID LOOP////////////////////////////////////////////
void door_way()
{
  giatri = digitalRead(cambien); //Đọc giá trị digital từ cảm biến và gán vào biến giatri

  if (giatri == 1)
  {
    digitalWrite(den, LOW);
  }
  else 
  {
    digitalWrite(den, HIGH);
  }
  delay(200);
}
void realtime()
{
    lcd.setCursor(0,1);
    lcd.print(" ");
    DateTime now = rtc.now();
    lcd.setCursor(0,0);
    lcd.print(" DATE:");
    lcd.print(now.day(), DEC);
    lcd.print("/");
    lcd.print(now.month(), DEC);
    lcd.print("/");
    lcd.print(now.year(), DEC);
    ///////////////////
    lcd.setCursor(2,1);
    lcd.print(" TIME:");
    
    int b = now.hour();
    int a = now.minute();
    
      if (b < 10){
      lcd.setCursor(8,1); lcd.print("0");
      lcd.setCursor(9,1); lcd.print(now.hour(), DEC);
      lcd.print(":");
      }
      else {
                 lcd.print(now.hour(), DEC);
                 lcd.print(":"); }
      if ( a < 10){
      lcd.setCursor(11,1); lcd.print("0");
      lcd.setCursor(12,1); lcd.print(now.minute(), DEC);
      }
      else {
                 lcd.print(now.minute(), DEC);
            }      
    delay(1000);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  
    {
    lcd.clear();
    lcd.print("VAN TAY SAI!");
    lcd.setCursor(0,1);
    lcd.print("Xin thu lai!");
    delay(1000);
    lcd.clear();
    return -1;
    }
    
  Firebase.setInt(firebaseData, path + "ID_check", finger.fingerID ); 
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); 
  Serial.println(finger.confidence);
  lcd.clear();
  lcd.print("Found ID:");
  lcd.setCursor(0,1);
  lcd.print(finger.fingerID);
  delay(500);
  lcd.clear();
  
  digitalWrite(relay, 1);
  delay(500);
  lcd.setCursor(3,0);
  lcd.print("XIN CAM ON");
  delay(1500);
  
  digitalWrite(relay, 0);
  return finger.fingerID; // relay control
}
void getPassword()
{
    for (int i=0 ; i<4 ; i++){
    c[i]= keypad.waitForKey();
    lcd.setCursor(i,1);
    lcd.print("*");
   }
   lcd.clear();
   for (int j=0 ; j<4 ; j++){ //comparing the code entred with the code stored
    if(c[j]==code[j])
     ij++;                    //Everytime the char is correct we increment the ij until it reaches 4 which means the code is correct
   }
}

void Enrolling()  {
  char temp = keypad.getKey();
  temp = NULL;
  lcd.clear();
  lcd.print("Enroll New");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter new ID");
  id = readnumber();
  Firebase.setInt(firebaseData, path + "ID_add", id ); 
  Firebase.setInt(firebaseData, path + "Danhsach/ID: " + id,  id );                         
  if (id == 0) {// ID #0 not allowed, try again!
  return;
  }
  getFingerprintEnroll();
  }

uint8_t readnumber(void) {
  uint8_t num = 0;
   while (num == 0) {
      char keey = keypad.waitForKey();
    int  num1 = keey-48;
         lcd.setCursor(0,1);
         lcd.print(num1);
         keey = keypad.waitForKey();
    int  num2 = keey-48;
         lcd.setCursor(1,1);
         lcd.print(num2);
         keey = keypad.waitForKey();
    int  num3 = keey-48;
         lcd.setCursor(2,1);
         lcd.print(num3);
         delay(1000);
         num=(num1*100)+(num2*10)+num3;
         keey=NO_KEY;
  }
  return num;
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  delay(500);
  lcd.clear();
  lcd.print("Dat ngon tay vao");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }
  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  

  lcd.clear();
  lcd.print("Lay ngon tay ra");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  
  p = -1;
  lcd.clear();
  lcd.print("Dat ngon tay lai");  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lcd.clear();
    lcd.print("Them thanh cong");
    Firebase.setInt(firebaseData, path + "ID_ADD", 0 );
    Firebase.setInt(firebaseData, path + "ID_add", 0 );
    
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}

void DeleteFP()  {
  char temp = keypad.getKey();
  temp = NULL;
  lcd.clear();
  lcd.print("Xoa van tay");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Nhap ID");
  id = readnumber();
  Firebase.setInt(firebaseData, path + "ID_xoa", id );                           
  if (id == 0) {// ID #0 not allowed, try again!
  return;
  }
  deleteFingerprint(id);
  }

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    lcd.clear();
    lcd.print("Da xoa ID:");
    lcd.setCursor(0,1);
    lcd.print(id);
    Firebase.setInt(firebaseData, path + "ID_DEL", 0 );
    delay(1000);
    Firebase.setInt(firebaseData, path + "ID_xoa", 0 );
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
   
}
int web_add()
{
  if(Firebase.getInt(firebaseData,"ID_ADD"))
  {
    int web_id_add = firebaseData.intData();
    if (web_id_add > 0)
    {
      id = web_id_add;
      getFingerprintEnroll();
    }
  }
}

int web_del()
{
  if(Firebase.getInt(firebaseData,"ID_DEL"))
  {
    int web_id_del = firebaseData.intData();
    if (web_id_del > 0)
    {
      id = web_id_del;
      deleteFingerprint(id);
    }
  }
}
