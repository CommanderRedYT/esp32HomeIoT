//#define _DEBUG_ //uncomment to enable debug-mode
//
//User Settings

    //WiFi Settings
    #define SSID ""          //Your WiFI-SSID
    #define SSID_PASSWORD "" //Your WiFi-Password

    //Thinger.io
    #define USERNAME ""          //Put your thinger.io username here
    #define DEVICE_ID ""         //Put the device-id here
    #define DEVICE_CREDENTIAL "" //Put the device-password here

    //Sinric.com
    #define MyApiKey "" //Put your sinric-api-key here
    #define DEVICE ""   //Put your sinric-device-id here

    //WakeOnLan
    const char *MACAddress = "YOUR MAC ADDRESS"; //Put your MAC-Address here

    //Pin Settings
    byte SEND_PIN = 5; //Pin Number where IR-LED is connected
    int RELAY_PIN = 4; //Pin Number where Relay is connected

    //Serial Baud Rate
    #define BAUD_RATE 115200
//User Settings end here

#include <Arduino.h>
#include <ThingerESP32.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <IRremote.h>

bool led = false;
bool wol;
bool ledstripToggle;
bool ledstripBrighter;
bool ledstripDarker;
bool avPower;
bool avVolup;
bool avVoldown;
bool avMute;
bool avDVD;
bool avTuner;
bool avAux;
bool avPresetUp;
bool avPresetDown;
unsigned int dataNEC = 0x000000;
unsigned int dataSamsung = 0x00000000;

//Notwendig?
const char *ssid = SSID;
const char *password = SSID_PASSWORD;

#define HEARTBEAT_INTERVAL 300000
uint64_t heartbeatTimestamp = 0;
bool isConnected;

WiFiMulti wifiMulti;
WebSocketsClient webSocket;
WiFiClient client;
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
WiFiUDP UDP;
WakeOnLan WOL(UDP);

IRsend irsend(SEND_PIN);

void setup()
{
    Serial.begin(BAUD_RATE);
    pinMode(RELAY_PIN, OUTPUT);
    thing.add_wifi(SSID, SSID_PASSWORD);
    thing["led"] << [](pson &in) {
        led = in;
    };
    thing["wol"] << [](pson &in) {
        wol = in;
        if (wol == true)
        {
            WOL.sendMagicPacket(MACAddress);
            Serial.println("Magic packet was successfuly sent");
            Serial.println("");
            wol = false;
        }
    };
    thing["webledstripToggle"] << [](pson &in) {
        ledstripToggle = in;
        IRledstripToggle();
        ledstripToggle = false;
    };
    thing["webledstripBrighter"] << [](pson &in) {
        ledstripBrighter = in;
        IRledstripBrighter();
        ledstripBrighter = false;
    };
    thing["webledstripDarker"] << [](pson &in) {
        ledstripDarker = in;
        IRledstripDarker();
        ledstripDarker = false;
    };
    thing["webavPower"] << [](pson &in) {
        avPower = in;
        IRavPower();
        avPower = false;
    };
    thing["webavVolup"] << [](pson &in) {
        avVolup = in;
        IRavVolup();
        avVolup = false;
    };
    thing["webavVoldown"] << [](pson &in) {
        avVoldown = in;
        IRavVoldown();
        avVoldown = false;
    };
    thing["webavMute"] << [](pson &in) {
        avMute = in;
        IRavMute();
        avMute = false;
    };
    thing["webavDVD"] << [](pson &in) {
        avDVD = in;
        IRavDVD();
        avDVD = false;
    };
    thing["webavTuner"] << [](pson &in) {
        avTuner = in;
        IRavTuner();
        avTuner = false;
    };
    thing["webavAux"] << [](pson &in) {
        avAux = in;
        IRavAux();
        avAux = false;
    };
    thing["webavPresetUp"] << [](pson &in) {
        avPresetUp = in;
        IRavPresetUp();
        avPresetUp = false;
    };
    thing["webavPresetDown"] << [](pson &in) {
        avPresetDown = in;
        IRavPresetDown();
        avPresetDown = false;
    };
    delay(10);
    wifiMulti.addAP(SSID, SSID_PASSWORD);
    webSocket.begin("iot.sinric.com", 80, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setAuthorization("apikey", MyApiKey);
    webSocket.setReconnectInterval(5000);
    WOL.setRepeat(3, 100);
}

void turnOn()
{
    led = true;
}

void turnOff()
{
    led = false;
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        isConnected = false;
        break;
    case WStype_CONNECTED:
        isConnected = true;
        break;
    case WStype_TEXT:
    {
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject((char *)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char *)payload);
#endif
        String deviceID = json["deviceID"];
        String action = json["action"];

        if (action == "setPowerState")
        {
            String value = json["value"];
            if (value == "ON")
            {
                turnOn();
            }
            else
            {
                turnOff();
            }
        }
    }
    break;
    }
}

void loop() {
    thing.handle();
    digitalWrite(RELAY_PIN, led);
    webSocket.loop();
    if(isConnected) {
        uint64_t now = millis();
        if ((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
            heartbeatTimestamp = now;
            webSocket.sendTXT("H");
        }
    }
}

void IRledstripToggle() {
  dataNEC = 0xFF02FD;
  sendNECcode();
}
void IRledstripBrighter() {
  dataNEC = 0xFF9867;
  sendNECcode();
}
void IRledstripDarker() {
  dataNEC = 0xFF18E7;
  sendNECcode();
}
void IRavPower() {
  dataNEC = 0x5EA1F807;
  sendNECcode();
}
void IRavVolup() {
  dataNEC = 0x5EA158A7;
  sendNECcode();
}
void IRavVoldown() {
  dataNEC = 0x5EA1D827;
  sendNECcode();
}
void IRavMute() {
  dataNEC = 0x5EA138C7;
  sendNECcode();
}
void IRavDVD() {
  dataNEC = 0x5EA1837C;
  sendNECcode();
}
void IRavTuner() {
  dataNEC = 0x5EA16897;
  sendNECcode();
}
void IRavAux() {
  dataNEC = 0x5EA1AA55;
  sendNECcode();
}
void IRavPresetUp() {
  dataNEC = 0x5EA108F7;
  sendNECcode();
}
void IRavPresetDown() {
  dataNEC = 0x5EA18877;
  sendNECcode();
}
void sendNECcode() {
  irsend.sendNEC(dataNEC, 32);
}