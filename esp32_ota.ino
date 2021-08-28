#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"
#include <MFRC522.h> 
#include <SPI.h> 


#include <WebServer.h>
#include <ArduinoOTA.h>
#include "time.h"

const char * ssid = "Tekwill";
const char * password = "";


String FirmwareVer = {
  "1.4"
};

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16

#define URL_fw_Version "https://raw.githubusercontent.com/comendantcristian/update_esp32/main/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/comendantcristian/update_esp32/main/fw.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"
int freq = 2000;
int channel = 0;
int resolution = 8;
long timpul;
int ore; int minute; int secunde; 
String myStrings[100][20] = {}; String l;
String ggg; String month; String sapt; String day; String hour; String minutes; String seconds; String ora; String date; String timp2; String timp1; String q; String timp1_sec; 
int e = 0; int ex = 0; int c  = 0;
bool LED1status = LOW;
char timeDay[3]; char timeMonth[15]; 
char timeHour[3]; char timeMinute[3]; char timeSeconds[3]; char timeWeekDay[10];

const char* www_username = "Stefan";
const char* www_password = "rs2021s";
const char* www_realm = "Custom Auth Realm";
String authFailResponse = "Authentication Failed";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 10800;

byte nuidPICC[4];

//IPAddress local_ip(192, 168, 1, 1);
//IPAddress gateway(192, 168, 1, 1);
//IPAddress subnet(255, 255, 255, 0);

WebServer server(80); 
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
MFRC522 mfrc522(SS_PIN, RST_PIN);

void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();

unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long interval = 60000;
const long mini_interval = 1000;

void repeatedCall() {
  static int num=0;
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }
  }
  if ((currentMillis - previousMillis_2) >= mini_interval) {
    previousMillis_2 = currentMillis;
    Serial.print("idle loop...");
    Serial.print(num++);
    Serial.print(" Active fw version:");
    Serial.println(FirmwareVer);
   if(WiFi.status() == WL_CONNECTED) 
   {
       Serial.println("wifi connected");
   }
   else
   {
    connect_wifi();
   }
  }
}

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button_boot = {
  0,
  0,
  false
};


void IRAM_ATTR isr() {
  button_boot.numberKeyPresses += 1;
  button_boot.pressed = true;
}


void setup() {
  //pinMode(button_boot.PIN, INPUT);
  //attachInterrupt(button_boot.PIN, isr, RISING);
  Serial.begin(115200);
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  //pinMode(LED_BUILTIN, OUTPUT);
  WiFi.mode(WIFI_STA);

  connect_wifi();

  pinMode(25, OUTPUT);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  //server.on("/", handle_root);
  ArduinoOTA.begin();
  server.on("/", []() {
    if (!server.authenticate(www_username, www_password))
      
    {
      return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    }
    LED1status = LOW;
    //server.send(200, "text/plain", "Login OK");
    server.send(200, "text/html", getPage(LED1status) );
  });
  
  server.on("/set1", handle_seton);
  server.on("/set0", handle_setoff);
  server.on("/get", handle_input); 
  server.onNotFound(handle_NotFound);
  server.begin();
  
  Serial.println("HTTP server started");
  delay(100);

  SPI.begin(); // Init SPI bus
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(12, channel);
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
  mfrc522.PCD_Init();
   
  Serial.println("Approach your reader card...");
  Serial.println();

}
void loop() {
//  if (button_boot.pressed) { //to connect wifi via Android esp touch app 
//    Serial.println("Firmware update Starting..");
//    firmwareUpdate();
//    button_boot.pressed = false;
//  }
  server.handleClient();
  LocalTime();
  
  
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
  return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
  return;
  }
  
  
  if(LED1status){c++;}
  if (c == 1){
   c = 0;
   writingData();
   
  }
  if(q != date){
    for (int i = 0; i < 100; i++){
      myStrings[i][7] = "0:0:0";
      myStrings[i][8] = "";
    }
    
  }
  q = date;

  if(l != month){
    for (int i = 0; i < 100; i++){
      myStrings[i][11] = "0:0:0";
      myStrings[i][12] = "";
    }
    
  }
  l = month;

  if(sapt == "Monday"){
    for (int i = 0; i < 100; i++){
      myStrings[i][9] = "0:0:0";
      myStrings[i][10] = "";
    }
    
  }
  

  
  readingData();
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  repeatedCall();
}

void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
}



void readingData()
{
  
  String codul_card;
  for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = mfrc522.uid.uidByte[i];
      codul_card = codul_card + nuidPICC[i];
    }
    
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  //buffer for read data
  byte buffer[SIZE_BUFFER] = {0};
 
  byte block = 1;
  String nume = "";
  
  byte size = SIZE_BUFFER; 
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); 
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Authentication failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
 
  status = mfrc522.MIFARE_Read(block, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Reading failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  
  
  

  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {
    if((char)buffer[i] != ' '){
       nume = nume + (char)buffer[i];
    }
  }
    e = 0;
    for (int i = 0; i < 100; i++){
      
      
      if (myStrings[i][0] == nume){
        ledcWriteTone(channel, 1000);
        digitalWrite(25, LOW);   // turn the LED on (HIGH is the voltage level)
                      
        
        digitalWrite(26,LOW);
        delay(200);
        ledcWriteTone(channel, 0);
        digitalWrite(26,HIGH);
        delay(3000);
        digitalWrite(25, HIGH);
        //delay(4000);
        
        
        e = 1;
        myStrings[i][4] = date;
        myStrings[i][3] = ora;
        if (myStrings[i][1] == "OUT"){
          myStrings[i][1] = "IN";
          myStrings[i][6] = "";
          myStrings[i][5] = String(timpul);
          //if (myStrings[i][4] )
        }
        else{
          myStrings[i][1] = "OUT";
          timp1_sec = String(timpul - myStrings[i][5].toInt());
          ore = timp1_sec.toInt() / 3600;
          secunde = timp1_sec.toInt() - ore * 3600;
          minute = secunde / 60;
          secunde = timp1_sec.toInt() - minute * 60;
          timp1 = String(ore) + ":" + String(minute) + ":" + String(secunde);
          myStrings[i][6] = timp1;
          
          myStrings[i][8] = String(myStrings[i][8].toInt() + timp1_sec.toInt());
          ore = myStrings[i][8].toInt() / 3600;
          secunde = myStrings[i][8].toInt() - ore * 3600;
          minute = secunde / 60;
          secunde = myStrings[i][8].toInt() - minute * 60;
          myStrings[i][7]  = String(ore) + ":" + String(minute) + ":" + String(secunde);

          myStrings[i][10] = String(myStrings[i][10].toInt() + timp1_sec.toInt());
          ore = myStrings[i][10].toInt() / 3600;
          secunde = myStrings[i][10].toInt() - ore * 3600;
          minute = secunde / 60;
          secunde = myStrings[i][10].toInt() - minute * 60;
          myStrings[i][9]  = String(ore) + ":" + String(minute) + ":" + String(secunde);

          myStrings[i][12] = String(myStrings[i][12].toInt() + timp1_sec.toInt());
          ore = myStrings[i][12].toInt() / 3600;
          secunde = myStrings[i][12].toInt() - ore * 3600;
          minute = secunde / 60;
          secunde = myStrings[i][12].toInt() - minute * 60;
          myStrings[i][11]  = String(ore) + ":" + String(minute) + ":" + String(secunde);


          
        }
        
      }
    }
   

      if (e == 0){
        for (int i = 0; i < 100; i++){
          if(myStrings[i][2] == codul_card){
            ex = 1;
            myStrings[i][0] = nume;
            myStrings[i][1] = "IN";
            myStrings[i][4] = date;
            myStrings[i][3] = ora;
            myStrings[i][5] = String(timpul);
            myStrings[i][6] = "";
            myStrings[i][7] = "0:0:0";
            myStrings[i][9] = "0:0:0";
            myStrings[i][11] = "0:0:0";
            myStrings[i][8] = "";
            myStrings[i][10] = "";
            myStrings[i][12] = "";
            ledcWriteTone(channel, 1000);
            digitalWrite(26,LOW);
            delay(200);
            ledcWriteTone(channel, 0);
            digitalWrite(26,HIGH);
            break;
          }
        }
        if (ex == 0){
          for (int i = 0; i < 100; i++){
            if (myStrings[i][2] == ""){
              myStrings[i][2] = codul_card;
              myStrings[i][1] = "IN";
              myStrings[i][7] = "0:0:0";
              myStrings[i][9] = "0:0:0";
              myStrings[i][10] = "0:0:0";
              myStrings[i][0] = nume;
              myStrings[i][4] = date;
              myStrings[i][3] = ora;
              myStrings[i][6] = "";
              myStrings[i][8] = "";
              myStrings[i][12] = "";
              myStrings[i][11] = "0:0:0";
              myStrings[i][5] = String(timpul);
              break;
            }
          }
        }
      }
      ex = 0;
}

void writingData(){
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  
  Serial.setTimeout(30000L) ;
  Serial.println(F("Enter the data to be written with the '#' character at the end \n[maximum of 16 characters]:"));
  
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  byte buffer[MAX_SIZE_BLOCK] = "";
  byte block; 
  byte dataSize;
  
  ggg = ggg + "#";
  ggg.toCharArray((char*)buffer, ggg.length());

  block = 1; 
  String str = (char*)buffer; 
  Serial.println(str);
  
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
  block, &key, &(mfrc522.uid));
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    
    return;
  }
  
  status = mfrc522.MIFARE_Write(block, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

    
    return;
  }
  else{
    Serial.println(F("MIFARE_Write() success: "));
    ledcWriteTone(channel, 800);
    delay(2000);
    ledcWriteTone(channel, 0);
  }

  
}


String getPage(uint8_t led1stat){
      
  String page = "<html lang=en-EN>";
  page +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  page += "<title>ESP32 WebServer</title>";
  page +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  page +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  page += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  page +=".button-on {background-color: #3498db;}\n";
  page +=".button-on:active {background-color: #2980b9;}\n";
  page +=".button-off {background-color: #34495e;}\n";
  page +=".button-off:active {background-color: #2c3e50;}\n";
  page +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  page += "</style>";
  page += "</head><body><h1>Cardurile de acces</h1>";
  page += "<h3>Ultimul refresh: "+month+" " + day+ " " + hour+ ":"+minutes+":"+seconds+"</h3>";
  
  page += "<h3>Setare Card Nou</h3>";

  if(led1stat)
    {
      page +="<a class=\"button button-on\" href=\"/set0\">Ready</a>\n";
      ledcWriteTone(channel, 800);
      delay(50);
      ledcWriteTone(channel, 0);
      delay(50);
      ledcWriteTone(channel, 800);
      delay(50);
      ledcWriteTone(channel, 0);
      delay(50);
      ledcWriteTone(channel, 800);
      delay(50);
      ledcWriteTone(channel, 0);
    }
  else
  {
      page +="<a class=\"button button-on\" href=\"/set1\">Set</a>\n";
  }

  page += "<form action='/get'>";
  page += "<label for='input1'>Numele:  </label>";
  page += "<input type='text'  name='input1' value='' required><br>";
  page += "<br>";
  page += "<input type='submit' value='Submit'>";
  page += "</form>";


  page += "<h3>Lucratorii</h3>";
  page += "<table border = 1 align = center><tr>";
  page += "<th>Nume</th><th>Prezenta</th><th>Ora</th><th>Data</th><th>Timpul lucrat</th><th>Timpul lucrat (Zi)</th> <th>Timpul lucrat (Saptamana)</th> <th>Timpul lucrat (Luna)</th></tr>";

  for(int i = 0; i < 100; i++){
    if (myStrings[i][0] != ""){
      page += "<tr>";
      page += "<th>" + myStrings[i][0] + "</th>";
      if (myStrings[i][1] == "IN"){
        page += "<th><font color='green'>" + myStrings[i][1] + "</font></th>";
      }

      if (myStrings[i][1] == "OUT"){
        page += "<th><font color='red'>" + myStrings[i][1] + "</font></th>";
      }

    
      
      
      page += "<th>"+  myStrings[i][3] +"</th>";
      page += "<th>" + myStrings[i][4] + "</th>";
      page += "<th>" + myStrings[i][6] + "</th>";
      page += "<th>" + myStrings[i][7] + "</th>";
      page += "<th>" + myStrings[i][9] + "</th>";
      page += "<th>" + myStrings[i][11] + "</th>";
      page += "</tr>";
    }
  }
  
  page += "</table>";
  page += "</body></html>";

  return page;
}

void handle_root() {
  LED1status = LOW;
  server.send(200, "text/html", getPage(LED1status) );
}

void handle_seton() {
  LED1status = HIGH;
  Serial.println("GPIO4 Status: ON");
  server.send(200, "text/html", getPage(true)); 
}

void handle_setoff() {
  LED1status = LOW;
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", getPage(false)); 
}
void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handle_input(){
  server.send(200, "text/html", getPage(false));
  ggg = server.arg("input1");
}

void LocalTime(){
  struct tm timeinfo;
  String timp;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  strftime(timeWeekDay,10, "%A", &timeinfo);
  strftime(timeDay,3, "%d", &timeinfo);
  strftime(timeMonth,15, "%B", &timeinfo);
  strftime(timeHour,3, "%H", &timeinfo);
  strftime(timeMinute,3, "%M", &timeinfo);
  strftime(timeSeconds,3, "%S", &timeinfo);

  sapt = (char*)timeWeekDay;
  month = (char*)timeMonth;
  day = (char*)timeDay;
  hour = (char*)timeHour;
  minutes = (char*)timeMinute;
  seconds = (char*)timeSeconds;

  date = month + " " + day;
  ora = hour + ":" + minutes + ":" + seconds;

  timpul = hour.toInt()*3600 + minutes.toInt()*60 + seconds.toInt();
  
  
}
