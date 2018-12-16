#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <DS1307.h>
#include "msTime.h"

msTime currentTime;
RTC_DS1307 rtc;

#define SENSOR_PIN D4
OneWire SensorWire(SENSOR_PIN);
DallasTemperature Sensor(&SensorWire);

#define FIREBASE_HOST "YOUR_HOST"
#define FIREBASE_AUTH "YOUR_SECRET_TOKEN"

const char *softAP_ssid = "SwitchController";
const char *softAP_password = "12345678";

const char *loginAccount = "admin";
const char *loginPassword = "admin";

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "esp8266";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char uid[128] = "EdUkId3pQvbNVyWcXWCGCk0QQDU2";
char ssid[32] = "";
char password[32] = "";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

/** Current WLAN status */
int status = WL_IDLE_STATUS;

struct Timer {
  msTime time;
  bool isEnabled;
  bool transitted = false;
  //0-00:00
  void parseString(String str) {
    str.trim();
    isEnabled = str.substring(0, str.indexOf("-")).toInt() == 1;
    int hour = str.substring(str.indexOf("-") + 1, str.indexOf(":")).toInt();
    int minute = str.substring(str.indexOf(":") + 1, str.length()).toInt();
    time.setTime(Time_t(hour, minute));
  }

  String toString() {
    String str = "";
    str += isEnabled ? "1" : "0";
    str += "-";
    str += time.getTime().Hour < 10 ? "0" : "";
    str += String(time.getTime().Hour);
    str += ":";
    str += time.getTime().Minute < 10 ? "0" : "";
    str += String(time.getTime().Minute);
    return str;
  }
};

Timer timerOn[4], timerOff[4];
uint8_t devicePins[4] = { D5, D6, D7, D10 };

float ReadTemperature() {
  Sensor.requestTemperatures();
  return Sensor.getTempCByIndex(0);
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int connRes = WiFi.waitForConnectResult();
  Serial.print("connRes: ");
  Serial.println(connRes);
}

void setup() {
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(devicePins[i], OUTPUT);
    digitalWrite(devicePins[i], HIGH);
  }
  Serial.begin(115200);
  Serial.println();
  Serial.setTimeout(3);
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  //With password
  WiFi.softAP(softAP_ssid, softAP_password);
  //Without password
  //WiFi.softAP(softAP_ssid);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/uidsave", handleUidSave);
  server.on("/generate_204", handleRoot); //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot); //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound(handleNotFound);

  //here the list of headers to be recorded
  const char * headerkeys[] = { "User-Agent", "Cookie" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);

  server.begin(); // Web server start
  Serial.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  Wire.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
}
void loop() {
  if (connect) {
    Serial.println("Connect requested");
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  int s = WiFi.status();
  if (s == 0 && millis() > (lastConnectTry + 60000)) {
    /* If WLAN disconnected and idle try to connect */
    /* Don't set retry time too low as retry interfere the softAP operation */
    connect = true;
  }
  if (status != s) { // WLAN status change
    Serial.print("Status: ");
    Serial.println(s);
    status = s;
    if (s == WL_CONNECTED) {
      /* Just connected to WLAN */
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      // Setup MDNS responder
      if (!MDNS.begin(myHostname)) {
        Serial.println("Error setting up MDNS responder!");
      } else {
        Serial.println("mDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
    } else if (s == WL_NO_SSID_AVAIL) {
      WiFi.disconnect();
    }
  }
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();

  handleSerial();
  if (s == WL_CONNECTED) {

    static uint32_t lastPrintIpMillis = 0;
    if (millis() > lastPrintIpMillis + 5000) {
      lastPrintIpMillis = millis();
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    }
    handleFirebase();
    handleClock();
  }
}

void handleSerial() {
  if (!Serial.available())
    return;
  String s = Serial.readString();
  int hour = s.substring(0, s.indexOf(":")).toInt();
  int minute = s.substring(s.indexOf(":") + 1, s.lastIndexOf(":")).toInt();
  int second = s.substring(s.lastIndexOf(":") + 1, s.length()).toInt();

  msTime setTime(Time_t(hour, minute, second));
  Serial.print("set time: ");
  Serial.println(setTime.toTimeString());

  rtc.adjust(DateTime(2018, 12, 15, setTime.getTime().Hour, setTime.getTime().Minute, setTime.getTime().Second));
}

void handleClock() {
  static uint32_t lastUpdateMillis = 0;
  if (millis() > lastUpdateMillis + 1000) {
    lastUpdateMillis = millis();

    DateTime now = rtc.now();
    currentTime.setTime(Time_t(now.hour(), now.minute(), now.second()));

    Serial.println(currentTime.toTimeString());

    if (!rtc.isrunning())
      return;

    for (uint8_t i = 0; i < 4; i++) {
      Epoch_t distance = (currentTime - timerOn[i].time).getEpoch();
      if (distance > 0 && distance < 20) {
        if (!timerOn[i].transitted) {
          timerOn[i].transitted = true;

          if (!timerOn[i].isEnabled)
            continue;

          digitalWrite(devicePins[i], LOW);
          String updates = "1>" + timerOn[i].toString() + ">" + timerOff[i].toString();
          Serial.println("updates: " + updates);
          Firebase.setString(String("users/") + String(uid) + "/device" + String(i), updates);
          // handle error
          if (Firebase.failed()) {
            Serial.print("firebase failed:");
            Serial.println(Firebase.error());
          }
        }
      }
      else {
        timerOn[i].transitted = false;
      }

      distance = (currentTime - timerOff[i].time).getEpoch();
      if (distance > 0 && distance < 20) {
        if (!timerOff[i].transitted) {
          timerOff[i].transitted = true;

          if (!timerOff[i].isEnabled)
            continue;

          digitalWrite(devicePins[i], HIGH);
          String updates = "0>" + timerOn[i].toString() + ">" + timerOff[i].toString();
          Serial.println("updates: " + updates);
          Firebase.setString(String("users/") + String(uid) + "/device" + String(i), updates);
          // handle error
          if (Firebase.failed()) {
            Serial.print("firebase failed:");
            Serial.println(Firebase.error());
          }
        }
      } else {
        timerOff[i].transitted = false;
      }
    }
  }
}
void handleFirebase() {
  static uint32_t lastUpdateMillis = 0;
  if (millis() > lastUpdateMillis + 2000) {
    lastUpdateMillis = millis();

    String device0 = Firebase.getString(String("users/") + String(uid) + "/device0");
    String device1 = Firebase.getString(String("users/") + String(uid) + "/device1");
    String device2 = Firebase.getString(String("users/") + String(uid) + "/device2");
    String device3 = Firebase.getString(String("users/") + String(uid) + "/device3");
    Firebase.setFloat(String("users/") + String(uid) + "/temperature", ReadTemperature());
    Firebase.setString(String("users/") + String(uid) + "/clock", currentTime.toTimeString());

    parseDeviceString(device0, 0);
    parseDeviceString(device1, 1);
    parseDeviceString(device2, 2);
    parseDeviceString(device3, 3);

    // handle error
    if (Firebase.failed()) {
      Serial.print("firebase failed:");
      Serial.println(Firebase.error());
    }
  }
}

void parseDeviceString(String str, uint8_t deviceIndex) {
  String temp = str.substring(0, str.indexOf(">"));
  bool state = temp.toInt() == 1;
  temp = str.substring(str.indexOf(">") + 1, str.lastIndexOf(">"));
  timerOn[deviceIndex].parseString(temp);
  Serial.print("timerOn" + String(deviceIndex) + "=" + timerOn[deviceIndex].toString());
  temp = str.substring(str.lastIndexOf(">") + 1, str.length());
  timerOff[deviceIndex].parseString(temp);
  Serial.print(", timerOff" + String(deviceIndex) + "=" + timerOff[deviceIndex].toString());
  Serial.println(String(", state=") + (state == true ? "ON" : "OFF"));

  digitalWrite(devicePins[deviceIndex], !state);
}
