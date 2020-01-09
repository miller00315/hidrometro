#include<Arduino.h>
#include<EEPROM.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "esp_system.h"

//Definimos o tamanho de memoria que pretendemos acessar
#define EEPROM_SIZE 8
//Endereço para leitura da EEPROM
#define EEPROM_ADDRESS 0

#define FIREBASE_HOST ""

#define FIREBASE_AUTH ""

#define LED 2

FirebaseData firebaseData;
//Váriavel que armazena a medida em litros que pretendemos calcular
long liters = 0;

char* ssid = "";
char* password = "";

String path = "/mensures";

struct FlowSensor {
  const uint8_t PIN;
  uint32_t numberRead;
};

FlowSensor sensor = {18, 0};

void IRAM_ATTR isr(){
  sensor.numberRead++;
}

const int loopTimeCtl = 0;
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule(){
    ets_printf("(watchdog) reiniciar\n"); //imprime no log
    esp_restart(); //reinicia o chip
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);

  pinMode(LED,OUTPUT);

  delay(500);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 3000000, true);
  timerAlarmEnable(timer);

  digitalWrite(LED,HIGH);

  delay(1000);

  digitalWrite(LED,LOW);
  
  liters = EEPROMReadlong(EEPROM_ADDRESS);

  connectWifi(ssid, password);
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  
  delay(10);
  attachInterrupt(sensor.PIN,isr, FALLING);
}

void connectWifi(char* rede, char* senha) {

  timerWrite(timer, 0);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(rede);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(rede, senha);

  while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED,HIGH);
      delay(250);
      Serial.print(".");
      digitalWrite(LED,LOW);
      delay(250);
      if(Serial.available() > 0){
       setupWifi(Serial.readString());
     }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void loop() {
    timerWrite(timer, 0);
    if(sensor.numberRead > 440){
      sensor.numberRead = 0;
      liters++;
      EEPROMWritelong(EEPROM_ADDRESS, liters);
      if (Firebase.pushInt(firebaseData, path, liters)){
        digitalWrite(LED,HIGH);
        Serial.println("Enviado: ");
        Serial.println("PATH: " + firebaseData.dataPath());
        delay(100);
        digitalWrite(LED,LOW);
      } else {
        Serial.println("Erro");
        Serial.println("REASON: " + firebaseData.errorReason());
      } 
    }

    if(Serial.available() > 0){
      setupWifi(Serial.readString());
    }
}

void EEPROMWritelong(int address, long value)
{
  byte four  = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two   = ((value >> 16) & 0xFF);
  byte one   = ((value >> 24) & 0xFF);
  
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);

  EEPROM.commit();
}

void setupWifi(String wifiData){

  if(wifiData.startsWith("wfc")){
    if(wifiData.indexOf('-') > 0){
      
      String rede = wifiData.substring(wifiData.indexOf('-')+1, wifiData.lastIndexOf('-'));
      String senha = wifiData.substring(wifiData.lastIndexOf('-')+1, wifiData.length()-1);
      connectWifi(string2char(rede), string2char(senha));
        
    }else{
      Serial.println("Formato de comando incorreto: utilize \"wfc-'ssid'-'senha'\"");
    }
  }else if(wifiData.startsWith("clear")){
    EEPROMClear(EEPROM_SIZE);
  }else{
    Serial.println("Comando desconhecido: " + wifiData);
  }
  
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four  = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two   = EEPROM.read(address + 2);
  long one   = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return 
  ((four << 0)  &  0xFF) + 
  ((three << 8) &  0xFFFF) + 
  ((two << 16)  &  0xFFFFFF) + 
  ((one << 24)  &  0xFFFFFFFF);
}

void EEPROMClear(int EEPROMSize) {
  for(int i = 0; i < EEPROMSize; i++){
    EEPROM.write(i,0);
  }
  EEPROM.commit();
}
