#include <SoftwareSerial.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>

Adafruit_SSD1306 display(0);
SoftwareSerial ATM328(D5, D6);

#define Taster D7

#define IPSize        16
#define variableSize 255

unsigned long oldMillis = 0;
unsigned long keyPressMillis = 0;

bool oldKeyState = HIGH;
bool counterReset = false;

String incomingStr;

unsigned int ZaehlerWert = 0;

String configFilename = "sysconf.json";

char ccuip[IPSize] = "";
char variable[variableSize]  = "";

//WifiManager - don't touch
bool shouldSaveConfig        = false;
#define wifiManagerDebugOutput   true
char ip[IPSize]      = "0.0.0.0";
char netmask[IPSize] = "0.0.0.0";
char gw[IPSize]      = "0.0.0.0";
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
  startOTAhandling();
}

void loop() {
  ArduinoOTA.handle();

  if (ATM328.available()) {
    incomingStr = ATM328.readString();
    if (incomingStr.startsWith(";")) {
      incomingStr = incomingStr.substring(1,incomingStr.length());
      incomingStr = incomingStr.substring(0,incomingStr.indexOf(";"));
      Serial.println("incomingStr = " + incomingStr);
      if (incomingStr.toInt() < 2147483646) {
        ZaehlerWert = ZaehlerWert + incomingStr.toInt();
        saveSysConfig();
        if (!setStateCCUCUxD(variable, String(ZaehlerWert))) {
          setStateCCUCUxD(variable, String(ZaehlerWert));
        }
      }
    }
    ATM328.print("ACK;ACK;ACK");
  }

  if (digitalRead(Taster) == LOW) {
    if (!counterReset) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(WHITE, BLACK);
      display.println("Hold to");
      display.println("reset");
      display.println("counter");
      display.display();
    }

    if (oldKeyState == HIGH) {
      oldKeyState = LOW;
      keyPressMillis = millis();
    }
    if (millis() - keyPressMillis > 5000 && !counterReset) {
      ZaehlerWert = 0;
      saveSysConfig();
      if (!setStateCCUCUxD(variable, String(ZaehlerWert))) {
        setStateCCUCUxD(variable, String(ZaehlerWert));
      }
      display.println( ""); display.println( "- OK -"); display.display();
      counterReset = true;
    }
  } else {
    oldKeyState = HIGH;
    counterReset = false;
  }

  if (millis() - oldMillis > 1000 && digitalRead(Taster) != LOW) {
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
}
