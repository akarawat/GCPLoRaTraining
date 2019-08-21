/*
  LoRaMaster Send data to UDP Server
  code.tommy.com
  coder : tommy
  created : 2019.Aug.19
  BAND 433E6, 868E6
  #V100
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "SSD1306.h"

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND  868E6

SSD1306 display(0x3c, 4, 15);

void setup() {
  pinMode(16, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Sender Test");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("init ok");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  delay(1500);
}

String pkg;
String n, t, h;
void loop() {
  pkg = "";
  int packetSize = LoRa.parsePacket();
  //Exam n01|24|77
  if (packetSize) {
    Serial.print("Received packet '");
    while (LoRa.available()) {
      //Serial.print((char)LoRa.read());
      pkg += String((char)LoRa.read());
    }
    // print RSSI of packet
    n = pkg.substring(0, 3);
    t = pkg.substring(4, 6);
    h = pkg.substring(7, 10);
    Serial.println(pkg + "Len:" + pkg.length());
    Serial.println("Split: " + n + " " + t + " " + h);
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Receiving packet: " + n);
    display.drawString(10, 15, "Temp:    " + t);
    display.drawString(10, 25, "Humidity:    " + h);
    display.drawString(10, 35, "RSSI:    " + String(LoRa.packetRssi()));
    display.drawString(10, 45, "By Tommy");
    display.display();

  }
}
