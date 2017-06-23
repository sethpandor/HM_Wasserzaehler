#include <SoftwareSerial.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

Adafruit_SSD1306 display(0);
SoftwareSerial ATM328(D5, D6);

#define Taster D7

unsigned long oldMillis = 0;

String incomingStr;

int ZaehlerWert = 0;

String configFilename = "sysconf.json";

char ccuip[15] = "";
char variable[255]  = "";

//WifiManager - don't touch
bool shouldSaveConfig        = false;
#define wifiManagerDebugOutput   true
char ip[15]      = "0.0.0.0";
char netmask[15] = "0.0.0.0";
char gw[15]      = "0.0.0.0";
bool startWifiManager = false;

void setup() {
  ATM328.begin(9600);
  Serial.begin(115200);
  pinMode(Taster, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.display();
  display.println("Start...");
  display.display();

  Serial.println("Begin");
  if (digitalRead(Taster) == LOW) {
    Serial.println("Taster = LOW");
    startWifiManager = true;
  }

  if (!SPIFFS.begin()) {
    display.println("SPIFFS");
    display.println("mnt error");
    display.display();
    Serial.println("Failed to mount file system");
  } else {
    if (!loadSysConfig()) {
      Serial.println("Failed to load config");
      display.println("Failed to");
      display.println("load conf");
      display.display();
    } else {
      Serial.println("Config loaded");
      display.println("Config");
      display.println("loaded");
      display.display();
    }
  }

  if (doWifiConnect() == true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Wifi");
    display.println("Connected");
    display.display();
    setStateCCUCUxD(variable, String(ZaehlerWert));
  }
  else ESP.restart();

}

void loop() {

  if (millis() - oldMillis > 1000) {
    display.clearDisplay();
    display.setCursor(0, 5);
    display.setTextColor(BLACK, WHITE);
    display.println("          ");
    display.setTextColor(WHITE, BLACK);
    display.println("          ");
    for (int i = 10; i > ((String)ZaehlerWert).length(); i--)
    display.print(" ");
    display.println(ZaehlerWert);
    display.println("          ");
    display.setTextColor(BLACK, WHITE);
    display.println("          ");
    display.display();
  }

  if (ATM328.available() > 0) {
    incomingStr = ATM328.readString();
    ZaehlerWert = ZaehlerWert + incomingStr.toInt();
    saveSysConfig();
    setStateCCUCUxD(variable, String(ZaehlerWert));
    ATM328.print(incomingStr);
  }
}