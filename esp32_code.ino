#include <Arduino.h>

//WIFI Definitions
#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "abcd";
const char* password = "aaaaaaaa";
const char* serverUrl = "insert server URL";
int wifi_connected;



//Screen Definitions
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_MOSI   23   // Data
#define OLED_CLK    18   // Clock
#define OLED_DC     2    // SA0 / DC
#define OLED_RESET  4    // Reset
#define OLED_CS     5
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);



//Timekeeper Definitons
#include "RTClib.h"
RTC_DS3231 rtc;



//AD8232 Definitions
#define ECG_OUTPUT 35   
#define LO_MINUS   16
#define LO_PLUS    17
#define SDN_PIN    27
int ecgValue;



//SD Card Definitions
#include <SD.h>




// SD card SPI pin definitions
#define SD_CS   15  
#define SD_SCK  14
#define SD_MOSI 13
#define SD_MISO 12
File file;


//Switch Pin Definitions
#define SW1 32
#define SW2 33
#define SW3 25
#define SW4 26

//Switch Boolean Definitions and Interrupt Declarations
  // Flags set by interrupts
  volatile bool write_to_card = false;
  volatile bool transmit_websocket = false;
  volatile bool display_on = false;
  volatile bool wifi_on = false;

  // Interrupt Service Routines
  void IRAM_ATTR write_to_card_isr() { write_to_card = (digitalRead(SW1) == LOW);}
  void IRAM_ATTR transmit_websocket_isr() { transmit_websocket = (digitalRead(SW2) == LOW);}
  void IRAM_ATTR display_on_isr() { display_on = (digitalRead(SW3) == LOW);}
  void IRAM_ATTR wifi_on_isr() { wifi_on = (digitalRead(SW4) == LOW);}


//Websocket Definitions
#include <WebSocketsClient.h>
bool websocket_started = false;
WebSocketsClient webSocket;


void setup(){
  Serial.begin(115200);


  //Switch Pin Setup
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);
  // Attach interrupts on FALLING since INPUT_PULLUP means LOW = pressed
  attachInterrupt(digitalPinToInterrupt(SW1), write_to_card_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW2), transmit_websocket_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW3), display_on_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW4), wifi_on_isr, CHANGE);
  write_to_card_isr();
  transmit_websocket_isr();
  display_on_isr();
  wifi_on_isr();

  //Screen Setup
    if(!display.begin(SSD1306_SWITCHCAPVCC)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.setTextSize(1);      
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);     
    display.cp437(true);  


  
  //AD8232 Setup
    // Lead-off pins
    pinMode(LO_MINUS, INPUT);
    pinMode(LO_PLUS, INPUT);

    // Shutdown pin
    pinMode(SDN_PIN, OUTPUT);
    digitalWrite(SDN_PIN, LOW);   // LOW = AD8232 active

    // ADC setup
    analogReadResolution(12);     // ESP32 default (0–4095)
    analogSetAttenuation(ADC_0db); // Best range for ECG (~0–1V)

    Serial.println("AD8232 ECG Ready");


  //Timekeeper Setup
    Wire.begin(21, 22); // SDA = 21, SCL = 22

    if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
    }


  //SD Card Setup
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    Serial.println("Mounting SD card...");

    if (!SD.begin(SD_CS, SPI, 4000000)) {
      Serial.println("SD card mount failed!");
      return;
    }

    Serial.println("SD card mounted successfully.");

    // Create and write a file
    file = SD.open("/test.txt", FILE_APPEND);
    if (file) {
      file.println("Hello from ESP32!");
      Serial.println("Wrote test.txt");
    } else {
      Serial.println("Failed to open file for writing.");
    }


  //Add Initial Date and time to the sd card file
  
  DateTime now = rtc.now();  
  file.print("Date: ");
  file.print(now.year());
  file.print('/');
  file.print(now.month());
  file.print('/');
  file.print(now.day());
  file.print("  Time: ");
  file.print(now.hour());
  file.print(':');
  file.print(now.minute());
  file.print(':');
  file.println(now.second());
  
}







void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            Serial.println("Connected to WebSocket!");
            break;

        case WStype_DISCONNECTED:
            Serial.println("WebSocket disconnected!");
            break;

        case WStype_TEXT:
            Serial.printf("Received text: %s\n", payload);
            break;

        case WStype_BIN:
            Serial.println("Received binary data");
            break;
    }
}


void sd_write(int ecg_write_value){
  if(file){
    if (digitalRead(LO_PLUS) == 1 || digitalRead(LO_MINUS) == 1) {
      file.println("!! Lead off detected !!");
    } else {
      file.println(ecg_write_value);
    }
    file.flush();
  }
}

void send_websocket(int ecg_write_value){
  webSocket.loop();
  int32_t value = ecg_write_value;  
  webSocket.sendBIN((uint8_t*)&value, sizeof(value));
  Serial.printf("Sent integer: %d\n", value);
}


void connect_wifi(){
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  int tries = 20;
  while (WiFi.status() != WL_CONNECTED && tries > 0) {
    delay(500);
    tries--;
    Serial.print(".");
  }
  if(WiFi.status() == WL_CONNECTED){
    wifi_connected = true;
  }
  Serial.println("\nConnected!");
}


void loop(){
  ecgValue = analogRead(ECG_OUTPUT);
  

  if (wifi_on && !wifi_connected) {
    connect_wifi();
}

  if(write_to_card){
    //sd_write(ecgValue);
  }

  if(transmit_websocket){
    if (!websocket_started) {
    websocket_started = true;
    webSocket.beginSSL("ecg-empty-tree-7386.fly.dev", 443, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(2000);
    }
    if(wifi_connected){
      send_websocket(ecgValue);
    }
  }
  
  if(true){
    Serial.println("display on");
    display.clearDisplay();
    display.display();
    display.setCursor(0, 0);
    
    if(wifi_on){
      display.println("Wi-Fi On");
      if(wifi_connected){
        display.println("Wi-Fi Connected");
      }
    }
    if(transmit_websocket){
      display.println("Websocket Enabled");
      if(websocket_started){
        display.println("Websocket Connected");
      }
    }
    if(write_to_card){
      display.println("Writing to Card");
    }
    display.println("ECG Value: " + String(ecgValue));
    display.display();
  }  
  

  delay(10);
}