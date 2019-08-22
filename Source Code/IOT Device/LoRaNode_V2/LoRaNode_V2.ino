/*
  LoRaMaster Send data to UDP Server
  code.tommy.com
  coder : tommy
  created : 2019.Aug.19
  BAND 433E6, 868E6
  #V100
  #V200   Add Base64 Encrypt Decrytp
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "SSD1306.h"

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND  868E6

SSD1306 display(0x3c, 4, 15);

String nodeKey = "*&|";
String nodeNo = "lora01";

const long intval = 3;  /* set delay loop second */
unsigned long prevms = 0;

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

  prevms = millis();
  delay(1500);

}

int t, h;
void loop() {
  unsigned long curms = millis();
  if (curms - prevms >= intval * 1000) {
    t = random(20, 35);
    h = random(40, 80);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Sending packet: " + nodeNo);
    display.drawString(10, 15, "Temp: " + String(t));
    display.drawString(10, 25, "Humidity: " + String(h));
    display.drawString(0, 35, "---------------------------------");
    display.drawString(10, 45, "By Tommy");
    display.display();

    // send packet
    String strText = nodeKey + "|" + nodeNo + "|" + String(t) + "|" + String(h);
    // encrypt the data
    char buf[strText.length() + 1];
    String bufStr = strText;
    snprintf( buf, sizeof(buf), "%s", bufStr.c_str() );
    int inputLen = sizeof(buf);
    int encodedLen = base64_enc_len(inputLen);
    char encoded[encodedLen];
    base64_encode(encoded, buf, inputLen);
    //---Debug--->> 
    Serial.println(encoded);
    ////////
    LoRa.beginPacket();
    LoRa.print(encoded);
    LoRa.endPacket();
    prevms = curms;
  }
}
