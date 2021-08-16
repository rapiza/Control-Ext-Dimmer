#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h> // doc de PubSubClient https://pubsubclient.knolleary.net/api#callback

#define TIMER_INTERRUPT_DEBUG  3
#include "ESP32TimerInterrupt.h"
WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "Carlos";//"HUAWEI P Smart 2019"; //"Depar";
const char* password = "12345678"; //+-+/adgjl--++";
const char* mqttServer = "192.168.137.19";//"10.2.128.44";//"192.168.0.101"; //192.168.0.100
const int mqttPort = 1883;
//const char* mqttUser = "yourMQTTuser";
//const char* mqttPassword = "yourMQTTpassword";

//Variables - resistencias *************************************************************
ESP32Timer ITimer0(0);
ESP32Timer ITimer1(1);
ESP32Timer ITimer2(2);
ESP32Timer ITimer3(3);
volatile int interruptCounter;
portMUX_TYPE crossMux = portMUX_INITIALIZER_UNLOCKED;
const int syncPin = 23;
const int thyristorPin = 21; //reactor
const int pinDimmer2 = 22;   //tolva
volatile int valor  = 7650; //6 voltios //7700 da dos voltios
volatile int valor2 = 7650;
volatile int pot_res=0;   //valor para variar la pontencia de la resistencia de la tolva
volatile int pot_res2=0;  //valor para varia la potencia de la resistencia del reactor

void IRAM_ATTR gate2_off(void){
  digitalWrite(thyristorPin ,LOW); 
  ITimer2.stopTimer();
 }
void IRAM_ATTR gate2_pulse(void){
   digitalWrite(thyristorPin ,HIGH);  
   ITimer1.stopTimer();
   ITimer2.setInterval(100,gate2_off);
  }

void IRAM_ATTR gate_off(void){
  digitalWrite(pinDimmer2,LOW); 
  ITimer3.stopTimer();
 }
 
void IRAM_ATTR gate_pulse(void){
   digitalWrite(pinDimmer2,HIGH);  
   ITimer0.stopTimer();
   ITimer3.setInterval(100,gate_off);
  }
  
void IRAM_ATTR fnc_cruce(){
   portENTER_CRITICAL_ISR(&crossMux);
   interruptCounter++;
   //ITimer1.attachInterruptInterval(vlr,gate_pulse);
   portEXIT_CRITICAL_ISR(&crossMux);
  }

void setup() {
  Serial.begin(115200);
  setup_wifi(); //funcion para conectarte al wifi
  client.setServer(mqttServer, mqttPort);
  client.setCallback(on_message);
  pinMode(syncPin,INPUT);  
  pinMode(thyristorPin, OUTPUT);
  pinMode(pinDimmer2, OUTPUT);
  //attachInterrupt(digitalPinToInterrupt(syncPin),fnc_cruce,FALLING)
  }

bool in = false;
void loop() {
  // put your main code here, to run repeatedly:
  if(!client.connected()){
     reconnect();
     //in = true;
    }
    
  if(client.connected()){
    valor2 = map(pot_res2,0,100,7650,50);
    valor = map(pot_res,0,100,7650,50);
    if (interruptCounter > 0){
    portENTER_CRITICAL(&crossMux);
    ITimer1.attachInterruptInterval(valor2,gate2_pulse); //para el reactor
    ITimer0.attachInterruptInterval(valor,gate_pulse);  //para la tolva
    interruptCounter--;
    portEXIT_CRITICAL(&crossMux);
    }
    
    if (in == false){
      Serial.println("Se activa la interrupcion");
      digitalWrite(syncPin, LOW);
      attachInterrupt(digitalPinToInterrupt(syncPin),fnc_cruce,FALLING);
      in=true;
      }
    //Serial.print(pot_res);
    //Serial.print(",");
    //Serial.println(pot_res2);
   }
  client.loop(); 
}


void setup_wifi() {
  //Conectamos a una red Wifi
  delay(10);
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("Direccion IP:");
  Serial.println(WiFi.localIP());
}

void reconnect(){
  while (!client.connected()){
    Serial.println("Connecting to MQTT...");
    //Crea un cliente ID
    String clientId = "Micro_ESP32";
    clientId += String(random(0xffff),HEX);
    //Intentamos conectar
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      //Sucribirse a los temas 
      //client.subscribe("planta/tolva/val_Pro");
      client.subscribe("planta/tolva/res_elec");
      client.subscribe("planta/reactor/resi_elec");
      //client.subscribe("planta/reactor/motor");
      //client.subscribe("planta/reactor/val_alv");
    } else {
      Serial.print("failed with state");
      Serial.println(client.state());
      delay(2000);
    }
  }
  }
 
void on_message(char* topic, byte* payload, unsigned int leng){
  String incoming = "";
  for(int i=0; i< leng; i++){
    incoming += (char)payload[i];
    }
   incoming.trim();
  if(String(topic) == "planta/tolva/res_elec"){
    //Serial.print(topic);
    pot_res = atoi(incoming.c_str());
    //Serial.println(" Mensaje ->" + incoming);
   }

  if (String(topic) == "planta/reactor/resi_elec"){
    //Serial.print(topic);
    pot_res2 = atoi(incoming.c_str());
    //Serial.println(" Mensaje ->" + incoming);
   }
  }