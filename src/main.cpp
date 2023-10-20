#include <Arduino.h>
#include "pin_manager.h"
#include <rele_lib.h>
#include <SPI.h>
#include <LoRa.h>                             // https://github.com/sandeepmistry/arduino-LoRa
#include <DHTesp.h>

const float templimit = 15;
const float humlimit = 65;
const float velvientomax = 1.5; 

String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 4000;          // interval between sends


char helada = 'S';
// S: sin helada
//B: blanca
//N: negra
byte intensidad=0;
int AUTOMATICO=1, ENCENDIDO=0;


float velviento = 23.1;
TempAndHumidity dataext, dataint;
DHTesp dhtext, dhtint;

Relay rele(RELEPIN);

void sendMessage(String outgoing) {
  digitalWrite(LEDCOM, HIGH);

  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
  digitalWrite(LEDCOM, LOW);
}

void onReceive(int packetSize) {
  digitalWrite(LEDREC, HIGH);
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length
  
  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  // String que contiene los valores
  //String cadena2 = "AUTO : 0 | Encendido : 2";

  // Extrae la parte numérica del string
  AUTOMATICO     = incoming.substring(incoming.indexOf('O')+4).toInt();
  ENCENDIDO = incoming.substring(incoming.indexOf('o')+4).toInt();

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println(AUTOMATICO);
  Serial.println(ENCENDIDO);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  digitalWrite(LEDREC, LOW);
}


void init(){
  dhtint.setup(DHTDATA1, DHTesp::DHT22);
  dhtext.setup(DHTDATA2, DHTesp::DHT22);
  // override the default CS, reset, and IRQ pins (optional)
  pinMode(LEDCOM, OUTPUT);
  pinMode(LEDREC, OUTPUT);
  
}


void calculohelada(){
  if(dataext.temperature <= templimit){
    if(dataext.humidity <= humlimit){
      helada = 'N';
      intensidad = 3;  
    }
    else{
      helada = 'B';
      if(velviento > velvientomax){
        intensidad = 2;

      }
      else{
        intensidad = 1;
      }
    }
    rele.on();
  }
  else{
    intensidad = 0;
    rele.off();
  }
  /*
  Serial.print(helada);
  Serial.print(" / ");
  Serial.println(intensidad);
  */
}

void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex");
  LoRa.setPins(NSS, RESET, DIO0);// set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");
  init();
}

void actuadorRele(int Autom, int Encen){
  if (Autom==1){
    if(intensidad == 0){
      rele.off();
    }
    else{
      rele.on();
    }
  }
  else{
    if(Encen==1){
      rele.on();
    }
    else{
      rele.off();
    }
  }
}

void loop() {
  if (millis() - lastSendTime > interval) {
    
    char bufferaux[70];
    dataext = dhtext.getTempAndHumidity();
    dataint = dhtint.getTempAndHumidity();

    calculohelada();
    sprintf(bufferaux, "T : %.1f | H : %1.f | t : %1.f | h : %1.f | V = %1.f | %c = %d", dataext.temperature, dataext.humidity, dataint.temperature, dataint.humidity, velviento, helada, intensidad);
    String message = String(bufferaux);
    sendMessage(message);
    Serial.println("Sending " + message + " with messageid: " +msgCount);
    
    calculohelada();
    actuadorRele(AUTOMATICO, ENCENDIDO);
    lastSendTime = millis();            // timestamp the message
    //interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}


