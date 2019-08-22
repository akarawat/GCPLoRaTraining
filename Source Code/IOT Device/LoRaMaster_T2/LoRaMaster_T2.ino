/*
  LoRaMaster Send data to UDP Server
  code.tommy.com
  coder : tommy
  created : 2019.Aug.19
  BAND 433E6, 868E6
  #V100
  #V101   Add Wifi for UDP Service
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "SSD1306.h"

// For Wifi
#include <WiFi.h>
#include <WiFiUdp.h>
const char * networkName = "Linxens_AY4";
const char * networkPswd = "!Linxens4";
const char * udpAddress = "35.240.177.29";
const int udpPort = 5683;
boolean connected = false;
WiFiUDP udp;

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND  868E6

SSD1306 display(0x3c, 4, 15);

/*---------Start Base 64 Script---------*/
const char PROGMEM b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "abcdefghijklmnopqrstuvwxyz"
                                    "0123456789+/";

/* 'Private' declarations */
inline void a3_to_a4(unsigned char * a4, unsigned char * a3);
inline void a4_to_a3(unsigned char * a3, unsigned char * a4);
inline unsigned char b64_lookup(char c);

int base64_encode(char *output, char *input, int inputLen) {
  int i = 0, j = 0;
  int encLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];

  while (inputLen--) {
    a3[i++] = *(input++);
    if (i == 3) {
      a3_to_a4(a4, a3);

      for (i = 0; i < 4; i++) {
        output[encLen++] = pgm_read_byte(&b64_alphabet[a4[i]]);
      }

      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) {
      a3[j] = '\0';
    }

    a3_to_a4(a4, a3);

    for (j = 0; j < i + 1; j++) {
      output[encLen++] = pgm_read_byte(&b64_alphabet[a4[j]]);
    }

    while ((i++ < 3)) {
      output[encLen++] = '=';
    }
  }
  output[encLen] = '\0';
  return encLen;
}

int base64_decode(char * output, char * input, int inputLen) {
  int i = 0, j = 0;
  int decLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];


  while (inputLen--) {
    if (*input == '=') {
      break;
    }

    a4[i++] = *(input++);
    if (i == 4) {
      for (i = 0; i < 4; i++) {
        a4[i] = b64_lookup(a4[i]);
      }

      a4_to_a3(a3, a4);

      for (i = 0; i < 3; i++) {
        output[decLen++] = a3[i];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) {
      a4[j] = '\0';
    }

    for (j = 0; j < 4; j++) {
      a4[j] = b64_lookup(a4[j]);
    }

    a4_to_a3(a3, a4);

    for (j = 0; j < i - 1; j++) {
      output[decLen++] = a3[j];
    }
  }
  output[decLen] = '\0';
  return decLen;
}

int base64_enc_len(int plainLen) {
  int n = plainLen;
  return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int base64_dec_len(char * input, int inputLen) {
  int i = 0;
  int numEq = 0;
  for (i = inputLen - 1; input[i] == '='; i--) {
    numEq++;
  }

  return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
  a4[0] = (a3[0] & 0xfc) >> 2;
  a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
  a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
  a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char * a3, unsigned char * a4) {
  a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
  a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
  a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char b64_lookup(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 71;
  if (c >= '0' && c <= '9') return c + 4;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}
/*---------Start Base 64 End---------*/

void setup() {
  pinMode(16, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  Serial.begin(115200);

  while (!Serial);
  Serial.println();
  // For Wifi
  connectToWiFi(networkName, networkPswd);

  Serial.println("LoRa Sender");
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

String nodeKey = "*&|";
String pkg;
String key, n, t, h, rssi;
void loop() {
  pkg = "";
  int packetSize = LoRa.parsePacket();
  //Exam n01|24|77
  if (packetSize) {
    Serial.print("Received packet :");
    while (LoRa.available()) {
      //Serial.print((char)LoRa.read());
      pkg += String((char)LoRa.read());
    }
    /*---Decrypt Start---*/
    Serial.println(pkg);
    String strText = pkg;
    char buf[strText.length() + 1];
    String bufStr = strText;
    snprintf( buf, sizeof(buf), "%s", bufStr.c_str() );

    int input2Len = sizeof(buf);
    int decodedLen = base64_dec_len(buf, input2Len);
    char decoded[decodedLen];
    base64_decode(decoded, buf, input2Len);
    String tDecod = String(decoded);
    Serial.print(tDecod);
    /*---Decrypt End---*/


    key = tDecod.substring(0, 3);
    if (key == nodeKey) {
      n = tDecod.substring(4, 10);
      t = tDecod.substring(11, 13);
      h = tDecod.substring(14, 16);

      Serial.println(tDecod + " Len:" + tDecod.length());
      Serial.println("Split: " + n + " " + t + " " + h);
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());

      // UDP Sender
      if (connected) {
        udp.beginPacket(udpAddress, udpPort);
        udp.printf("%u:%u:%d:", t.toInt(), h.toInt(), LoRa.packetRssi());
        udp.printf(n.c_str());
        udp.printf(":qwerty1234");
        udp.endPacket();
      }

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
}

void connectToWiFi(const char * ssid, const char * pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  Serial.println("Waiting for WIFI connection...");
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      break;
  }
}
