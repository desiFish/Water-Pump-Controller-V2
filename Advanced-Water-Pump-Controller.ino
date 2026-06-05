/*
Advanced-Water-Pump-Controller.ino
Copyright (C) 2024 desiFish

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

Hardware:
1. DOIT ESP32 DEVKIT V1
2. 128x64 OLED Display
3. DS1307 RTC
4. SCT Current sensor
5. AJ-SR04 Ultrasonic Sensor (Water Proof)
6. Float Sensor (Any)

NOT USING CURRENTLY--> ZMPT101B Voltage Sensor
*/

// For basic ESP32 stuff like wifi, OTA Update and Wifi Manager Server
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <LittleFS.h>

// For Display and I2C
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Fonts/FreeSerif9pt7b.h"

// Data Storage
#include <Preferences.h>

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include "RTClib.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// RGB LED (2812B)
#include <Adafruit_NeoPixel.h>

// For SCT013 Current Sensor
#include "EmonLib.h"

// Google Sheet Logging
#include <HTTPClient.h>

// JSON handling for API
#include <ArduinoJson.h>

EnergyMonitor emon1;

// PIN CONFIGURATIONS
#define BUTTON 15
// #define VOLTAGE_SENSOR 34
#define UltraRX 16 // to TX of sensor
#define UltraTx 17 // to RX of sensor
// SDA 21, SCL 22 used for I2C Devices
#define LED_PIN 4 // for WS2812B RGB LED
#define BUZZER_PIN 5
#define FLOAT_SENSOR 36
#define PUMP_PIN 2            // PUMP RELAY PIN
#define CURRENT_SENSOR_PIN 39 // SCT SENSOR PIN

// Output Devices
#define TURN_ON_RELAY digitalWrite(PUMP_PIN, HIGH) // update this
#define TURN_OFF_RELAY digitalWrite(PUMP_PIN, LOW) // update this

/*3 is 3 seconds, you can assign any time value you wish.
 This is given because it takes a while for the current consumption to get stable.
 And there are all sort of current and voltage spikes just after the pump is ON
 Giving it few seconds should stabilize the reading. Min. value of 3 secs is suggested.*/
#define WAIT_AFTER_PUMP_ON 3

#define NUM_LEDS 1
// Define the LED object
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial uSonicSerial(2);
#define uSonic_BAUD 9600
#define MAX_ULTRASONIC_VALUE 400 // 400cm or 4meters or 4000 mm max distance read for this model (update accordingly)
#define TANK_VOLUME 950          // tank will never fill up to the brims due to sensors, for 1000 L I am reducing 50 L (approx) (update accordingly)

// Variables to hold sensor readings
byte percBegin;
byte percEnd;

// Variable to save current epoch time
String startTime, endTime;

Preferences pref;

RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800); // 19800 is offset of India, asia.pool.ntp.org is close to India 5.5*60*60

// your wifi name and password (used in preference)
String ssid;
String password;

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

// Your Domain name with URL path or IP address with path
const char *serverName = "http://iotthings.pythonanywhere.com/api/pump_logs"; // this is my custum server which recieves the values from ESP32 and stores them in Google sheet
String apiKey;

// For Display
#define i2c_Address 0x3c // initialize with the I2C addr 0x3C Typically eBay OLED's
// #define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Manager HTML served from LittleFS (data/wifimanager.html)
// HTTP POST parameter names
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

// variables tankLow for storing ultrasonic value for empty tank and tankFull for full level.
int tankLow, tankFull, liveTankLevel;
// variables ampLow for lowest safe level and ampMax for safe ampere max value.
float ampLow, ampMax;
float liveAmp, sumAmp;
int countAmp;
// pump status
bool isPumpRunning = false;
// float sensor status
bool floatSensor = false;
// using sensors or not
bool useUltrasonic, useSensors, useFloat, useWifi;
bool resetFlag = false, updateInProgress = false;

String errorCodeMessage[] = {"USR INTRPT", "TANK FULL", "HIGH AMPERE", "LOW AMPERE"};
// time and timer related variables
byte timeHour, timeMinute;
time_t pumpStartTime = 0; // timestamp when pump starts (for elapsed time calculation)
int autoRunTimes[3][2] = {
    {615, 730},
    {1229, 1330},
    {1615, 1715}};
const char *autoRunTimeKeys[3][2] = {
    {"onTime1", "offTime1"},
    {"onTime2", "offTime2"},
    {"onTime3", "offTime3"}};
int lastDay;
byte doneForToday = 0, activeAutoRunPeriod = 0;
bool autoRun, isDisplayOn;
String dateAndTime, currTime;
// for holding water level (in %)
byte holdData = 0;
// global error tracking variable, Core 0 updates it
byte raiseAlert = 0;

// display update frequency
unsigned long previousMillis = 0; // will store last time it was updated
long interval = 1000;             // interval to wait (milliseconds)

// ultrasonic update frequency
unsigned long previousMillis1 = 0; // will store last time it was updated
long interval1 = 2000;             // interval to wait (milliseconds)

// float update frequency
unsigned long previousMillis2 = 0; // will store last time it was updated
long interval2 = 1000;             // interval to wait (milliseconds)

// Display auto-off feature (like smartphone)
unsigned long displayAutoOffTime = 30000; // Display turns off after 30 seconds of inactivity (in milliseconds)
unsigned long lastButtonPressTime = 0;    // Track when button was last pressed

TaskHandle_t loop2Code;

// Elegant OTA related task
void onOTAStart()
{
  isDisplayOn = true;
  // Log when OTA has started
  updateInProgress = true;
  pixels.setBrightness(200);
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
  Serial.println("OTA update started!");
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(9, 10);
  display.println("OTA UNDER PROGRESS");
  display.display();
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
  // Log
  if (millis() - ota_progress_millis > 500)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(9, 10);
    display.println("OTA UNDER PROGRESS");
    display.setCursor(0, 25);
    display.println("Done:");
    display.print(current);
    display.println(" bytes");
    display.setCursor(0, 45);
    display.println("Total:");
    display.print(final);
    display.println(" bytes");
    display.display();
  }
}

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {
    Serial.println("OTA update finished successfully!");
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(2, 10);
    display.println("OTA UPDATE SUCCESSFUL");
    display.display();
  }
  else
  {
    Serial.println("There was an error during OTA update!");
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(13, 10);
    display.println("OTA UPDATE FAILED");
    display.display();
  }
  // <Add your own code here>
  updateInProgress = false;
  isDisplayOn = false;
}

// forward declaration
void drawTankLevel(byte);
void blinkOrange(byte, byte, int = 50);
bool autoTimeUpdate();
void pumpRunSequence(bool = false);

enum PumpStatus : byte
{
  STATUS_OK = 0,
  STATUS_NEEDS_WATER = 1,

  ALERT_TANK_FULL = 2,
  ALERT_OVERCURRENT = 3,
  ALERT_UNDERCURRENT = 4,

  ALERT_AUTOSTART = 99
};

/**
 * @brief Initializes the pump controller system
 *
 * This function sets up all necessary hardware components, loads settings,
 * and prepares the system for operation.
 */
void setup(void)
{
  Serial.begin(115200);
  pinMode(PUMP_PIN, OUTPUT);
  TURN_OFF_RELAY;
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
  pixels.begin();
  pixels.setBrightness(100);
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();

  // Initialize LittleFS for serving web files
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed");
  }
  else
  {
    Serial.println("LittleFS Mounted Successfully");
  }

  pinMode(BUTTON, INPUT);
  pref.begin("database", false);
  display.begin(i2c_Address, true);
  display.setContrast(0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setFont(&FreeSerif9pt7b);
  display.setCursor(5, 15);
  display.println("INITIALIZING");
  display.setCursor(37, 35);
  display.println("PUMP");
  display.setCursor(0, 55);
  display.println(" CONTROLLER");
  display.display();
  delay(500);

  // loading preset values from the memory
  if (!pref.isKey("tankLow"))
    pref.putInt("tankLow", 0);
  if (!pref.isKey("tankFull"))
    pref.putInt("tankFull", 0);
  if (!pref.isKey("ampLow"))
    pref.putFloat("ampLow", 0.0);
  if (!pref.isKey("ampMax"))
    pref.putFloat("ampMax", 0.0);
  if (!pref.isKey("useUltrasonic"))
    pref.putBool("useUltrasonic", false);
  if (!pref.isKey("useSensors"))
    pref.putBool("useSensors", false);
  if (!pref.isKey("useFloat"))
    pref.putBool("useFloat", false);
  if (!pref.isKey("useWifi"))
    pref.putBool("useWifi", true);
  if (!pref.isKey("apiKey"))
    pref.putString("apiKey", "");
  if (!pref.isKey("doneToday"))
    pref.putUChar("doneToday", 0);
  if (!pref.isKey("lastDay"))
    pref.putInt("lastDay", 0);
  if (!pref.isKey("autoRun"))
    pref.putBool("autoRun", false);
  for (byte i = 0; i < 3; i++)
  {
    if (!pref.isKey(autoRunTimeKeys[i][0]))
      pref.putInt(autoRunTimeKeys[i][0], autoRunTimes[i][0]);
    if (!pref.isKey(autoRunTimeKeys[i][1]))
      pref.putInt(autoRunTimeKeys[i][1], autoRunTimes[i][1]);
  }

  tankLow = pref.getInt("tankLow", 0);
  tankFull = pref.getInt("tankFull", 0);
  ampLow = pref.getFloat("ampLow", 0);
  ampMax = pref.getFloat("ampMax", 0);
  useUltrasonic = pref.getBool("useUltrasonic", false);
  useSensors = pref.getBool("useSensors", false);
  useFloat = pref.getBool("useFloat", false);
  useWifi = pref.getBool("useWifi", true);
  apiKey = pref.getString("apiKey", "");
  doneForToday = pref.getUChar("doneToday", 0);
  lastDay = pref.getInt("lastDay", 0);
  autoRun = pref.getBool("autoRun", false);
  for (byte i = 0; i < 3; i++)
  {
    autoRunTimes[i][0] = pref.getInt(autoRunTimeKeys[i][0], autoRunTimes[i][0]);
    autoRunTimes[i][1] = pref.getInt(autoRunTimeKeys[i][1], autoRunTimes[i][1]);
  }

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    // Show countdown from 5 to 1 then move on
    for (int countdown = 5; countdown >= 1; countdown--)
    {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(34, 10);
      display.println("RTC FAILED");
      display.setCursor(50, 41);
      display.println(countdown);
      display.display();
      delay(1000);
    }
  }
  if (autoRun)
  {
    DateTime now = rtc.now();
    if (lastDay != now.day())
    { // if today is a different day, reset "doneForToday"
      lastDay = now.day();
      pref.putInt("lastDay", lastDay);
      pref.putUChar("doneToday", 0);
      doneForToday = 0;
      activeAutoRunPeriod = 0;
    }
  }

  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();

  if (useWifi)
  {
    // wifi manager
    bool wifiConfigExist = pref.isKey("ssid");
    if (!wifiConfigExist)
    {
      pref.putString("ssid", "");
      pref.putString("password", "");
    }

    ssid = pref.getString("ssid", "");
    password = pref.getString("password", "");

    if (ssid == "" || password == "")
    {
      Serial.println("No values saved for ssid or password");
      // Connect to Wi-Fi network with SSID and password
      Serial.println("Setting AP (Access Point)");
      // NULL sets an open Access Point
      WiFi.softAP("WIFI_MANAGER", "WIFImanager");

      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      wifiManagerInfoPrint();

      // Web Server WiFi Manager Route - Serve from LittleFS
      server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
                { 
                  if (LittleFS.exists("/wifimanager.html")) {
                    request->send(LittleFS, "/wifimanager.html", "text/html");
                  } else {
                    request->send(404, "text/plain", "WiFi Manager file not found. Please upload data files.");
                  } });

      server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request)
                {
        int params = request->params();
        for (int i = 0; i < params; i++) {
          const AsyncWebParameter *p = request->getParam(i);
          if (p->isPost()) {
            // HTTP POST ssid value
            if (p->name() == PARAM_INPUT_1) {
              ssid = p->value();
              Serial.print("SSID set to: ");
              Serial.println(ssid);
              pref.putString("ssid", ssid);
            }
            // HTTP POST pass value
            if (p->name() == PARAM_INPUT_2) {
              password = p->value();
              Serial.print("Password set to: ");
              Serial.println(password);
              pref.putString("password", password);
            }
          }
        }
        request->send(200, "text/plain", "Done. Device will now restart.");
        delay(3000);
        ESP.restart(); });
      server.begin();
      WiFi.onEvent(WiFiEvent);
      while (true)
        ;
    }

    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname("PumpController");
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5, 15);
    display.println("WAITING FOR");
    display.setCursor(30, 35);
    display.println("WIFI TO");
    display.setCursor(15, 55);
    display.println(" CONNECT");
    display.display();

    // count variable stores the status of WiFi connection. 0 means NOT CONNECTED. 1 means CONNECTED

    bool count = 1;
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      display.clearDisplay();
      display.setCursor(10, 15);
      display.println("COULD NOT");
      display.setCursor(20, 35);
      display.println("CONNECT");
      display.display();
      Serial.println("Connection Failed");
      delay(2000);
      // ESP.restart();
      count = 0;
      break;
    }
    if (count)
    {
      Serial.println(ssid);
      Serial.println(WiFi.localIP());
      display.clearDisplay();
      display.setCursor(11, 15);
      display.println("CONNECTED");
      display.setCursor(15, 35);
      display.println("IP ADDRESS");
      display.setCursor(20, 55);
      display.setFont(NULL);
      display.println(WiFi.localIP());
      display.display();
      delay(4000);
      display.clearDisplay();
      display.setCursor(5, 15);
      display.println("Setting");
      display.setCursor(5, 35);
      display.println("Server");
      display.display();

      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send(LittleFS, "/settings.html", "text/html"); });

      // GET Settings API
      server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request)
                {
          pref.begin("database", false);

          DynamicJsonDocument doc(2048);

          doc["tankLow"] = pref.getInt("tankLow", 0);
          doc["tankFull"] = pref.getInt("tankFull", 0);

          doc["ampLow"] = pref.getFloat("ampLow", 0);
          doc["ampMax"] = pref.getFloat("ampMax", 0);

          doc["useUltrasonic"] = pref.getBool("useUltrasonic", false);
          doc["useSensors"] = pref.getBool("useSensors", false);
          doc["useFloat"] = pref.getBool("useFloat", false);
          doc["useWifi"] = pref.getBool("useWifi", true);

          doc["autoRun"] = pref.getBool("autoRun", false);

          doc["apiKey"] = pref.getString("apiKey", "");

          doc["ssid"] = pref.getString("ssid", "");
          doc["password"] = pref.getString("password", "");

          doc["onTime1"] = pref.getInt("onTime1", 615);
          doc["offTime1"] = pref.getInt("offTime1", 730);

          doc["onTime2"] = pref.getInt("onTime2", 1229);
          doc["offTime2"] = pref.getInt("offTime2", 1330);

          doc["onTime3"] = pref.getInt("onTime3", 1615);
          doc["offTime3"] = pref.getInt("offTime3", 1715);

          String json;
          serializeJson(doc, json);

          pref.end();

          request->send(200, "application/json", json); });

      // POST Settings API
      AsyncCallbackJsonWebHandler *handler =
          new AsyncCallbackJsonWebHandler(
              "/api/settings",
              [](AsyncWebServerRequest *request, JsonVariant &json)
              {
                JsonObject data = json.as<JsonObject>();

                // Log received data from browser
                Serial.println("Settings received from browser:");
                serializeJson(data, Serial);
                Serial.println();

                pref.begin("database", false);

                tankLow = data["tankLow"] | 0;
                pref.putInt("tankLow", tankLow);

                tankFull = data["tankFull"] | 0;
                pref.putInt("tankFull", tankFull);

                ampLow = data["ampLow"] | 0.0;
                pref.putFloat("ampLow", ampLow);

                ampMax = data["ampMax"] | 0.0;
                pref.putFloat("ampMax", ampMax);

                useUltrasonic = data["useUltrasonic"] | false;
                pref.putBool("useUltrasonic", useUltrasonic);

                useSensors = data["useSensors"] | false;
                pref.putBool("useSensors", useSensors);

                useFloat = data["useFloat"] | false;
                pref.putBool("useFloat", useFloat);

                useWifi = data["useWifi"] | true;
                pref.putBool("useWifi", useWifi);

                autoRun = data["autoRun"] | false;
                pref.putBool("autoRun", autoRun);

                apiKey = String((const char *)data["apiKey"]);
                pref.putString("apiKey", apiKey);

                ssid = String((const char *)data["ssid"]);
                pref.putString("ssid", ssid);

                password = String((const char *)data["password"]);
                pref.putString("password", password);

                autoRunTimes[0][0] = data["onTime1"] | 615;
                pref.putInt("onTime1", autoRunTimes[0][0]);

                autoRunTimes[0][1] = data["offTime1"] | 730;
                pref.putInt("offTime1", autoRunTimes[0][1]);

                autoRunTimes[1][0] = data["onTime2"] | 1229;
                pref.putInt("onTime2", autoRunTimes[1][0]);

                autoRunTimes[1][1] = data["offTime2"] | 1330;
                pref.putInt("offTime2", autoRunTimes[1][1]);

                autoRunTimes[2][0] = data["onTime3"] | 1615;
                pref.putInt("onTime3", autoRunTimes[2][0]);

                autoRunTimes[2][1] = data["offTime3"] | 1715;
                pref.putInt("offTime3", autoRunTimes[2][1]);

                pref.end();

                request->send(200, "text/plain", "Settings Saved");
              });
      server.addHandler(handler);

      server.on("/api/live", HTTP_GET, [](AsyncWebServerRequest *request)
                {
    // Log live data request from browser
    Serial.println("Live data requested from browser");

    DynamicJsonDocument doc(1024);

    doc["tankPercent"] = tankLevelPerc();
    doc["ultrasonicDistance"] = liveTankLevel;
    doc["liveAmp"] = liveAmp;

    doc["floatSensor"] = floatSensor;
    doc["pumpRunning"] = isPumpRunning;

    doc["tankLow"] = tankLow;
    doc["tankFull"] = tankFull;

    doc["time"] = currTime;
    doc["dateTime"] = dateAndTime;

    // useful extra information
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;

    String json;
    serializeJson(doc, json);

    Serial.println("Live data sent to browser:");
    Serial.println(json);

    request->send(200, "application/json", json); });

      // Serve Settings HTML
      server.on("/settings", HTTP_GET,
                [](AsyncWebServerRequest *request)
                {
                  request->send(LittleFS, "/settings.html", "text/html");
                });
      server.on("/api/rtc/sync", HTTP_OPTIONS,
                [](AsyncWebServerRequest *request)
                {
                  request->send(200);
                });

      AsyncCallbackJsonWebHandler *rtcHandler =
          new AsyncCallbackJsonWebHandler(
              "/api/rtc/sync",
              [](AsyncWebServerRequest *request, JsonVariant &json)
              {
                JsonObject data = json.as<JsonObject>();

                int year = data["year"];
                int month = data["month"];
                int day = data["day"];
                int hour = data["hour"];
                int minute = data["minute"];
                int second = data["second"];

                Serial.printf("Time received from browser: %04d-%02d-%02d %02d:%02d:%02d\n",
                              year, month, day, hour, minute, second);

                rtc.adjust(DateTime(
                    year,
                    month,
                    day,
                    hour,
                    minute,
                    second));

                request->send(
                    200,
                    "text/plain",
                    "Received by ESP32");
              });
      server.addHandler(rtcHandler);
      server.on("/api/rtc/update", HTTP_POST,
                [](AsyncWebServerRequest *request)
                {
                  bool success = autoTimeUpdate();

                  if (success)
                  {
                    request->send(
                        200,
                        "text/plain",
                        "RTC synchronized successfully");
                  }
                  else
                  {
                    request->send(
                        500,
                        "text/plain",
                        "RTC synchronization failed");
                  }
                });

      server.on("/api/restart", HTTP_POST,
                [](AsyncWebServerRequest *request)
                {
                  resetFlag = true;
                  request->send(200, "text/plain", "Device restarting...");
                });

      ElegantOTA.begin(&server); // Start ElegantOTA
      // ElegantOTA callbacks
      ElegantOTA.onStart(onOTAStart);
      ElegantOTA.onProgress(onOTAProgress);
      ElegantOTA.onEnd(onOTAEnd);

      server.begin();
      Serial.println("HTTP server started");
      display.clearDisplay();
      display.setCursor(5, 15);
      display.println("Setting");
      display.setCursor(5, 35);
      display.println("Time");
      display.display();
      // RTC Update at startup
      autoTimeUpdate();
    }
  }

  pref.end();
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  uSonicSerial.begin(uSonic_BAUD, SERIAL_8N1, UltraRX, UltraTx);

  emon1.current(CURRENT_SENSOR_PIN, 27); // Current: input pin, calibration.
  analogReadResolution(10);              // read resolution (10=10 bits)

  // Initialize display auto-off timer - set to current time so display stays on initially

  // create a task that will be executed in the loop2() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
      loop2,       // Task function.
      "loop2Code", // name of task.
      10000,       // Stack size of task
      NULL,        // parameter of the task
      1,           // priority of the task
      &loop2Code,  // Task handle to keep track of created task
      0);          // pin task to core 0
}

/**
 * @brief Secondary core loop for sensor monitoring and safety checks
 *
 * @param pvParameters Pointer to task parameters (unused in this implementation)
 *
 * This function runs continuously on Core 0, handling real-time sensor monitoring
 * and safety-critical operations.
 */
void loop2(void *pvParameters)
{
  static uint32_t lastRtcUpdate = 0;
  static uint32_t lastAmpSample = 0;
  static uint32_t pumpStartMillis = 0;

  for (;;)
  {
    uint32_t currentMillis = millis();
    // =========================
    // Sensor updates
    // =========================

    if (useSensors)
      liveAmp = readAmpere();

    if (useFloat)
      floatSensor = readFloat();

    if (useUltrasonic)
      liveTankLevel = readUltrasonic();

    // =========================
    // Pump safety monitoring
    // =========================
    bool isInStartupPeriod;
    if (isPumpRunning)
    {
      // Track pump start time to allow initial current stabilization
      if (pumpStartMillis == 0)
      {
        pumpStartMillis = currentMillis;
      }

      uint32_t elapsedSincePumpStart = currentMillis - pumpStartMillis;
      isInStartupPeriod = (elapsedSincePumpStart < (WAIT_AFTER_PUMP_ON * 1000));

      byte err = monitorPumpSafety();

      // Immediate protection
      // Skip ampere-based errors during startup period to avoid inrush current shutdown
      if (err >= ALERT_TANK_FULL &&
          err <= ALERT_UNDERCURRENT)
      {
        if (!isInStartupPeriod || err == ALERT_TANK_FULL)
        {
          pumpStop();
        }
      }

      // Notify UI
      if (err >= ALERT_TANK_FULL && raiseAlert == STATUS_OK)
      {
        if (!isInStartupPeriod || err == ALERT_TANK_FULL) // only raise alert if not in startup period or if it's a tank full alert
          raiseAlert = err;
      }

      Serial.print("PUMP RUN ERRORCODE: ");
      Serial.println(err);
    }
    else
    {
      // Reset pump start timer when pump stops
      pumpStartMillis = 0;
    }

    // =========================
    // RTC update (1 second)
    // =========================

    if (currentMillis - lastRtcUpdate >= 1000)
    {
      lastRtcUpdate = currentMillis;

      DateTime now = rtc.now();

      timeHour = now.hour();
      timeMinute = now.minute();

      dateAndTime = now.timestamp(DateTime::TIMESTAMP_FULL);

      currTime = now.timestamp(DateTime::TIMESTAMP_TIME);

      if (autoRun && lastDay != now.day())
      {
        lastDay = now.day();
        doneForToday = 0;
        activeAutoRunPeriod = 0;
        pref.begin("database", false);
        pref.putInt("lastDay", lastDay);
        pref.putUChar("doneToday", doneForToday);
        pref.end();
      }
    }

    // =========================
    // Current averaging
    // =========================

    if (isPumpRunning && useSensors && useWifi && !isInStartupPeriod) // only sample current for averaging if pump is running, sensors are enabled, wifi is enabled (for logging) and not in startup period
    {
      if (currentMillis - lastAmpSample >= 1000)
      {
        lastAmpSample = currentMillis;

        sumAmp += liveAmp;
        countAmp++;
      }
    }

    // =========================
    // Automatic schedule
    // =========================

    if (autoRun && !isPumpRunning && raiseAlert == 0)
    {
      for (byte i = 0; i < 3; i++)
      {
        byte autoRunDoneFlag = 1 << i;
        if ((doneForToday & autoRunDoneFlag) == 0 &&
            checkTimeFor(autoRunTimes[i][0], autoRunTimes[i][1]))
        {
          activeAutoRunPeriod = i + 1;
          raiseAlert = ALERT_AUTOSTART;
          break;
        }
      }
    }

    // =========================
    // Display auto-off timer
    // =========================
    // Turn off display if pump is not running, display is on, and it's been more than displayAutoOffTime since last button press
    unsigned long timeSinceLastPress = currentMillis - lastButtonPressTime;
    if (isDisplayOn || isPumpRunning || timeSinceLastPress < displayAutoOffTime)
    {
      // Turn display on
      displayPower(true);
    }
    else
    {
      // Turn display off
      displayPower(false);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/**
 * @brief Main loop for user interface and non-critical operations
 *
 * This function runs on Core 1, managing the user interface, display updates,
 * and other non-critical tasks.
 */
void loop(void)
{
  // no OTA when pump is running
  if (!isPumpRunning && useWifi)
    ElegantOTA.loop();

  if (raiseAlert == ALERT_AUTOSTART)
  {
    isDisplayOn = true;
    raiseAlert = STATUS_OK;
    runPumpAuto();
    isDisplayOn = false;
  }

  if (raiseAlert >= ALERT_TANK_FULL &&
      raiseAlert <= ALERT_UNDERCURRENT)
  {
    isDisplayOn = true;
    handlePumpCompletion(raiseAlert);
    raiseAlert = STATUS_OK;
    isDisplayOn = false;
  }

  if (!updateInProgress)
  {
    if (resetFlag) // after resetting (set in menu options; reset), esp32 will restart
    {
      isDisplayOn = true;
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(5, 20);
      display.println("PUMP CONTROLLER WILL");
      display.setCursor(5, 40);
      display.print("RESTART NOW");
      display.display();

      pumpStop();
      digitalWrite(BUZZER_PIN, LOW);
      server.end();
      pref.end();
      WiFi.disconnect(true);
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      LittleFS.end();
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      delay(3000);
      ESP.restart();
    }

    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);

      display.setCursor(5, 1);
      display.print(currTime);

      display.setCursor(60, 1);
      if (isPumpRunning)
      {
        display.println("PUMP: ON");
        pixels.setPixelColor(0, pixels.Color(128, 0, 128));
        pixels.show();
      }
      else
      {
        display.println("PUMP: OFF");
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
        pixels.show();
      }

      if (useUltrasonic)
        drawTankLevel(tankLevelPerc());

      vitals();
      display.display();
    }
  }
  // long press to activate menu
  byte count = 0;
  if (digitalRead(BUTTON) == HIGH)
  {
    lastButtonPressTime = millis(); // Update last button press time on each press to keep display on
    if (!isDisplayOn)
    {
      delay(500); // Debounce delay
      isDisplayOn = true;
    }

    while (digitalRead(BUTTON) == HIGH)
    {
      count++;
      if (count >= 1 && count <= 15)
      {
        blinkOrange(1, 20, 50);
      }
      else
      {
        blinkOrange(0, 150, 0);
        delay(100);
      }
      delay(50);
    }

    pixels.setBrightness(100);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();

    if (count >= 1 && count <= 15)
    {
      pixels.setPixelColor(0, pixels.Color(255, 255, 0));
      pixels.show();
      pumpRunSequence();
    }
    else if (count > 15)
      menu();
    isDisplayOn = false;
  }
}

/**
 * @brief Controls the power state of the SH1106 OLED display
 *
 * @param on If true, turns the display on; if false, turns it off
 *
 * This function sends I2C commands to control the display power state via the SH1106 controller.
 * Optimized with early-return to avoid redundant I2C operations - only sends commands when
 * the display state actually changes, reducing bus traffic and improving efficiency.
 */
void displayPower(bool on)
{
  static bool currentState = true; // Assume display starts ON
  if (currentState == on)
    return; // No change needed
  Wire.beginTransmission(0x3C);
  Wire.write(0x00);
  if (on)
    Wire.write(0xAF); // Display ON
  else
    Wire.write(0xAE); // Display OFF
  Wire.endTransmission();
  currentState = on;
}
/**
 * @brief Centralized pump start control
 *
 * This function initializes all necessary variables and turns on the pump relay.
 * It should be called whenever the pump needs to be started.
 */
void pumpStart()
{
  percBegin = tankLevelPerc();
  isPumpRunning = true;
  TURN_ON_RELAY;
  pumpStartTime = time(NULL); // Set current time as pump start time
  holdData = 0;               // Reset tank level display smoothing
  startTime = currTime;

  if (useWifi)
  {
    countAmp = 0; // Reset ampere sample count
    sumAmp = 0;   // Reset ampere sum
  }

  Serial.println("PUMP STARTED");
}

/**
 * @brief Centralized pump stop control
 *
 * This function turns off the pump relay and resets all related tracking variables.
 * It should be called whenever the pump needs to be stopped.
 */
void pumpStop()
{
  TURN_OFF_RELAY;
  isPumpRunning = false;
  Serial.println("PUMP STOPPED");
}

/**
 * @brief Resets the pump operation timer
 *
 * This function clears the pump start time, allowing for a fresh elapsed time calculation
 * on the next pump start. It should be called whenever the pump operation needs to be reset.
 */
void resetTimer()
{
  pumpStartTime = 0;
}

/**
 * @brief Initiates or stops the pump operation with safety checks
 *
 * @param flag If true, initiates auto-run mode
 *
 * This function handles the pump start/stop sequence, including user confirmation
 * and safety checks before operation.
 */
void pumpRunSequence(bool flag)
{
  delay(100);
  byte count = 0, option = 1;
  while (true)
  {
    bool stopMode = isPumpRunning;

    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(30, 10);
    display.println(stopMode ? "STOP PUMP?" : "START PUMP?");

    if (option == 1)
    {
      display.setCursor(20, 40);
      display.print("YES");
      display.setCursor(90, 40);
      display.fillRect(88, 39, 15, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("NO");
      display.setTextColor(SH110X_WHITE);
    }
    else
    {
      display.setCursor(20, 40);
      display.fillRect(18, 39, 21, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("YES");
      display.setTextColor(SH110X_WHITE);
      display.setCursor(90, 40);
      display.print("NO");
    }

    if ((!stopMode && flag) || digitalRead(BUTTON) == 1)
    {
      while (digitalRead(BUTTON) == 1)
      {

        count++;
        if (count >= 1 && count <= 8)
        {
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      if (flag && !stopMode)
      {
        count = 50;
        option = 2;
      }

      // pixels.setBrightness(20);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 8)
      {
        option++;
        if (option > 2)
          option = 1;
      }
      else
      {
        if (option == 1)
          break; // NO

        if (stopMode)
        {
          pumpStop();
          handlePumpCompletion(1); // raise user interrupt error
          break;
        }

        byte errorCode = monitorPumpSafety();
        if (errorCode <= 1) // check if there is any error
        {
          pumpStart();
          pixels.setPixelColor(0, pixels.Color(0, 255, 255));
          pixels.show();
          pumpOnDelay();
        }
        else
        {
          handlePumpCompletion(errorCode); // raise error
        }
        break;
      }
    }
    count = 0;
    display.display();
  }
}

/**
 * @brief Monitors system parameters and returns error status
 *
 * @return byte Error code (0: No error, 1: Tank not full, 2: Tank full, 3: High ampere, 4: Low ampere)
 *
 * This function checks various sensor readings and system states to ensure safe operation.
 */
byte monitorPumpSafety()
{
  if (useFloat && floatSensor)
    return ALERT_TANK_FULL;

  if (useSensors && isPumpRunning)
  {
    if (liveAmp > ampMax)
      return ALERT_OVERCURRENT;

    if (liveAmp < ampLow)
      return ALERT_UNDERCURRENT;
  }

  if (useFloat && !floatSensor)
    return STATUS_NEEDS_WATER;

  return STATUS_OK;
}

/**
 * @brief Format elapsed time as HH:MM:SS string
 * @param elapsedSeconds Total elapsed seconds since pump started
 * @return Formatted string in HH:MM:SS format
 */
String formatElapsedTime(time_t elapsedSeconds)
{
  int hours = elapsedSeconds / 3600;
  int minutes = (elapsedSeconds % 3600) / 60;
  int seconds = elapsedSeconds % 60;

  String h = hours < 10 ? "0" + String(hours) : String(hours);
  String m = minutes < 10 ? "0" + String(minutes) : String(minutes);
  String s = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return h + ":" + m + ":" + s;
}

/**
 * @brief Displays a countdown after pump starts to allow current stabilization
 *
 * This function shows a countdown on the OLED display for a predefined duration
 * after the pump is turned on, allowing the current to stabilize before safety checks.
 */
void pumpOnDelay()
{

  for (byte secondsLeft = WAIT_AFTER_PUMP_ON; secondsLeft > 0; secondsLeft--)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(32, 10);
    display.println("PLEASE WAIT");
    display.setCursor(62, 35);
    display.print(secondsLeft);
    display.display();

    uint32_t secondStartedAt = millis();
    while (millis() - secondStartedAt < 1000)
    {
      delay(10);
      yield();
    }
  }
}

/**
 * @brief Automatically starts the pump after a countdown, allowing user to cancel
 *
 * This function initiates a 10-second countdown before automatically starting the pump.
 * During the countdown, the user can press the button to cancel the auto-start.
 */
void runPumpAuto()
{

  for (byte secondsLeft = 10; secondsLeft > 0 && !isPumpRunning; secondsLeft--)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(10, 5);
    display.print("AUTO-PUMP STARTING");
    display.setCursor(5, 25);
    display.print("PRESS BUTTON TO STOP");
    display.setCursor(62, 45);
    display.print(secondsLeft);
    display.display();

    pixels.setBrightness(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();

    uint32_t secondStartedAt = millis();
    while (millis() - secondStartedAt < 1000)
    {
      if (digitalRead(BUTTON) == 1)
      {
        if (activeAutoRunPeriod >= 1 && activeAutoRunPeriod <= 3)
        {
          doneForToday |= (1 << (activeAutoRunPeriod - 1));
          pref.begin("database", false);
          pref.putUChar("doneToday", doneForToday);
          pref.end();
        }
        activeAutoRunPeriod = 0;
        pixels.setPixelColor(0, pixels.Color(255, 165, 0));
        pixels.show();
        while (digitalRead(BUTTON) == 1)
        {
          delay(10);
          yield();
        }
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        return;
      }
      delay(10);
      yield();
    }

    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
  }
  pumpRunSequence(true);
}

constexpr uint16_t CURRENT_SAMPLES = 1480;

/**
 * @brief Reads current value from the ampere sensor
 *
 * @return float Current reading in amperes
 */
float readAmpere()
{
  return emon1.calcIrms(CURRENT_SAMPLES);
}

bool usWaitingForResponse = false;
uint32_t usTriggerTime = 0;
/**
 * @brief Reads distance value from the ultrasonic sensor
 *
 * @return int Distance reading in centimeters
 */
int readUltrasonic()
{
  static int distanceCm = 0;

  uint32_t currentMillis = millis();

  // Time to start a new measurement
  if (!usWaitingForResponse &&
      (currentMillis - previousMillis1 >= interval1))
  {
    previousMillis1 = currentMillis;

    // Clear old UART data
    while (uSonicSerial.available())
      uSonicSerial.read();

    uSonicSerial.write(0x55);

    usTriggerTime = currentMillis;
    usWaitingForResponse = true;
  }

  // Wait for sensor response without blocking
  if (usWaitingForResponse &&
      (currentMillis - usTriggerTime >= 70))
  {
    String response = "";

    while (uSonicSerial.available())
    {
      response += (char)uSonicSerial.read();
    }

    if (response.startsWith("Gap="))
    {
      int distanceMm = response.substring(4).toInt();

      if (distanceMm > 0 && distanceMm < 10000)
      {
        distanceCm = distanceMm / 10;
      }
    }

    usWaitingForResponse = false;
  }

  return distanceCm;
}

/**
 * @brief Reads the state of the float sensor
 *
 * @return bool True if water level is high, false otherwise
 */
bool readFloat()
{
  static bool floatState = false;

  uint32_t currentMillis = millis();

  if (currentMillis - previousMillis2 >= interval2)
  {
    previousMillis2 = currentMillis;

    // true = tank full
    // false = tank not full
    floatState = (analogRead(FLOAT_SENSOR) <= 900);
  }

  return floatState;
}

/**
 * @brief Displays the tank water level graphically
 *
 * @param x Water level percentage (0-100)
 *
 * This function renders a graphical representation of the tank water level on the OLED display.
 * It implements a simple smoothing algorithm to minimize fluctuations due to waves.
 */
void drawTankLevel(byte x)
{
  // static byte holdData = 0;
  const byte MAX_CHANGE = 3; // Maximum allowed change in percentage per update
  const byte MIN_CHANGE = 1; // Minimum change required to update display

  // Initialize holdData if it's the first run
  if (holdData == 0)
  {
    holdData = x;
  }

  // Calculate the difference between new and old values
  int difference = x - holdData;

  // Update holdData only if the change is within acceptable range
  if (abs(difference) >= MIN_CHANGE && abs(difference) <= MAX_CHANGE)
  {
    holdData = x;
  }
  else if (abs(difference) > (MAX_CHANGE))
  {
    // If change is too large, move holdData slightly towards the new value
    holdData += (difference > 0) ? 1 : -1;
  }

  // Ensure holdData stays within 0-100 range
  holdData = constrain(holdData, 0, 100);

  // Draw the tank outline
  display.drawRoundRect(3, 20, 40, 41, 3, 1);
  display.drawRoundRect(13, 18, 20, 3, 3, 1);

  // Fill the tank based on holdData
  byte fillHeight = map(holdData, 0, 100, 0, 33);
  display.fillRect(5, 57, 36, -fillHeight, 1);

  // Display the percentage
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  if (holdData == 100)
  {
    display.fillRect(11, 27, 25, 9, 1);
    display.setCursor(12, 28);
  }
  else
  {
    display.fillRect(14, 27, 19, 9, 1);
    display.setCursor(15, 28);
  }
  display.print(String(holdData) + "%");
  display.setTextColor(SH110X_WHITE);
}

/**
 * @brief Calculates the water level percentage in the tank
 *
 * @return byte The calculated water level percentage (0-100)
 *
 * This function uses the current ultrasonic sensor reading (liveTankLevel)
 * and compares it with the calibrated empty (tankLow) and full (tankFull) values
 * to calculate the current water level percentage in the tank.
 */
byte tankLevelPerc()
{
  // Check if calibration values are valid
  if (tankLow <= tankFull || tankLow == 0 || tankFull == 0)
  {
    Serial.println("Error: Invalid tank calibration values");
    return 0; // Return 0% if calibration is invalid
  }

  // Ensure liveTankLevel is within valid range
  int currentLevel = constrain(liveTankLevel, tankFull, tankLow);

  // Calculate percentage
  float percentage = map(currentLevel, tankLow, tankFull, 0, 100);

  // Round to nearest integer and constrain between 0 and 100
  int roundedPercentage = round(percentage);
  roundedPercentage = constrain(roundedPercentage, 0, 100);

  Serial.printf("Tank Level: %d%% (Raw: %d, Low: %d, Full: %d)\n",
                roundedPercentage, liveTankLevel, tankLow, tankFull);

  return (byte)roundedPercentage;
}

/**
 * @brief Displays vital system information on the OLED screen
 *
 * This function updates the OLED display with current system status information:
 * - Pump runtime (if pump is running)
 * - Float sensor status (if enabled)
 * - Current sensor readings (if enabled)
 * - Ultrasonic sensor readings (if enabled)
 *
 * The information displayed depends on which sensors are active and the current system state.
 */
void vitals()
{
  if (isPumpRunning)
  {
    display.setCursor(50, 12);
    display.print("Time:");
    time_t elapsedTime = time(NULL) - pumpStartTime;
    display.print(formatElapsedTime(elapsedTime));
  }

  if (useFloat)
  {
    if (floatSensor)
    {
      display.setCursor(62, 32);
      display.print("TANK FULL");
    }
    else
    {
      display.setCursor(65, 32);
      display.print("NOT FULL");
    }
  }

  if (useSensors)
  {
    display.setCursor(50, 42);
    display.print("Amp : " + String(liveAmp) + " A");
  }

  if (useUltrasonic)
  {
    int volume = map(tankLevelPerc(), 0, 100, 0, TANK_VOLUME);
    display.setCursor(50, 22);
    display.print("Vol : ~ " + String(volume) + " L");
    display.setCursor(50, 52);
    display.print("U.S.: " + String(liveTankLevel) + " cm");
  }
}

/// ========================= MENU SYSTEM =========================
void menu(void)
{
  delay(100);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("MENU");
    display.drawLine(0, 8, 127, 8, 1);
    display.setCursor(3, 13);
    display.print("1. WIFI");
    display.setCursor(3, 27);
    display.print("2. Reset Wifi");
    display.setCursor(3, 41);
    display.print("3. Exit");

    switch (option)
    {
    case 1:
      display.drawRect(0, 10, 127, 13, 1);
      break;
    case 2:
      display.drawRect(0, 24, 127, 13, 1);
      break;
    case 3:
      display.drawRect(0, 38, 127, 13, 1);
      break;
    }
    display.display();

    if (digitalRead(BUTTON) == 1)
    {

      while (digitalRead(BUTTON) == 1)
      {
        count++;
        if (count >= 1 && count <= 6)
        {
          blinkOrange(1, 20, 50);
        }
        else
        {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(60);
      }

      // pixels.setBrightness(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 6)
      {
        option++;
        if (option > 3)
          option = 1;
      }
      else
      {
        if (option == 1)
          powerWifi();
        else if (option == 2)
          resetWifi();
        else if (option == 3)
          return;
      }
    }
    count = 0;
  }
  display.clearDisplay();
}

/**
 * @brief Blinks the RGB LED in orange color
 *
 * @param times Number of times to blink (0 for continuous)
 * @param brightValue Brightness of the LED (0-255)
 * @param blinkDuration Duration of each blink in milliseconds (default 50ms)
 *
 * This function controls the RGB LED to create a blinking effect in orange color.
 * It's used for visual feedback in the user interface.
 */
void blinkOrange(byte times, byte brightValue, int blinkDuration)
{
  pixels.setBrightness(brightValue);

  if (times == 0)
  {
    pixels.setPixelColor(0, pixels.Color(255, 165, 0));
    pixels.show();
    return;
  }

  for (byte i = 0; i < times; i++)
  {
    pixels.setPixelColor(0, pixels.Color(255, 165, 0));
    pixels.show();
    delay(blinkDuration);

    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(blinkDuration);
  }
}

/**
 * @brief Displays the wifi power menu for wifi settings
 *
 * This function presents a menu-driven interface on the OLED display allowing users to
 * configure device settings such as WiFi connectivity. Users navigate through the menu
 * options using the button input and can toggle WiFi on/off or return to the main menu.
 * The function saves preference changes to persistent storage and triggers a device restart
 * when WiFi settings are modified.
 */
void powerWifi()
{
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("WIFI");
    display.drawLine(0, 8, 127, 8, 1);

    switch (option)
    {
    case 1:
      display.setCursor(25, 40);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(23, 39, 15, 10, 1);
      display.print("ON");
      display.setCursor(85, 40);
      display.setTextColor(SH110X_WHITE);
      display.print("OFF");
      break;
    case 2:
      display.setCursor(25, 40);
      display.setTextColor(SH110X_WHITE);
      display.print("ON");
      display.setCursor(85, 40);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(83, 39, 21, 10, 1);
      display.print("OFF");
      display.setTextColor(SH110X_WHITE);
      break;
    }
    display.display();

    if (digitalRead(BUTTON) == 1)
    {
      while (digitalRead(BUTTON) == 1)
      {
        count++;
        if (count >= 1 && count <= 6)
        {
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      // pixels.setBrightness(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 2)
      {
        option++;
        if (option > 2)
          option = 1;
      }
      else
      {
        if (option == 1)
        {
          useWifi = true;
          pref.putBool("useWifi", true);
          break;
        }
        else if (option == 2)
        {
          useWifi = false;
          pref.putBool("useWifi", false);
          break;
        }
      }
    }
    count = 0;
    resetFlag = true;
  }

  display.clearDisplay();
  pref.end();
}

/**
 * @brief Automatically synchronizes the RTC with NTP time via WiFi
 *
 * @return bool True if RTC was successfully synchronized with NTP time, false otherwise
 *
 * This function checks if the device is connected to WiFi, then requests the current time
 * from an NTP server and updates the RTC with the received time. This ensures the device
 * maintains accurate time even if the RTC battery has drained. Logs the synchronization
 * result to the serial monitor.
 */
bool autoTimeUpdate()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    timeClient.begin();
    if (timeClient.update())
    {
      time_t rawtime = timeClient.getEpochTime();
      struct tm *ti;
      ti = localtime(&rawtime);

      uint16_t year = ti->tm_year + 1900;
      uint8_t x = year % 10;
      year = year / 10;
      uint8_t y = year % 10;
      year = y * 10 + x;

      uint8_t month = ti->tm_mon + 1;

      uint8_t day = ti->tm_mday;
      rtc.adjust(DateTime(year, month, day, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));

      Serial.println("RTC synchronized successfully");
      return true;
    }
  }

  Serial.println("RTC synchronization failed");
  return false;
}

/**
 * @brief Displays a confirmation dialog to reset WiFi credentials
 *
 * This function presents a "Reset WIFI?" confirmation prompt on the OLED display.
 * If the user confirms by pressing the button, all stored WiFi credentials (SSID and password)
 * are cleared from persistent memory and the device restarts in WiFi Manager mode to allow
 * new WiFi configuration. If cancelled, the function returns without making changes.
 */
void resetWifi()
{
  byte count = 0, option = 1;
  pixels.setBrightness(250);
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
  while (true)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 10);
    display.println("Reset WIFI?");
    // buttons
    if (option == 1)
    {
      display.setCursor(20, 40);
      display.print("YES");
      display.setCursor(90, 40);
      display.fillRect(88, 39, 15, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("NO");
      display.setTextColor(SH110X_WHITE);
    }
    else
    {
      display.setCursor(20, 40);
      display.fillRect(18, 39, 21, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("YES");
      display.setTextColor(SH110X_WHITE);
      display.setCursor(90, 40);
      display.print("NO");
    }

    display.display();

    if (digitalRead(BUTTON) == 1)
    {

      while (digitalRead(BUTTON) == 1)
      {
        count++;
        if (count >= 1 && count <= 2)
        {
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      // pixels.setBrightness(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 2)
      {
        option++;
        if (option > 2)
          option = 1;
      }
      else
      {
        if (option == 1) // NO
          break;
        else if (option == 2)
        { // YES
          pref.begin("database", false);
          pref.putString("ssid", "");
          pref.putString("password", "");
          pref.end();
          resetFlag = true;
        }
      }
    }
    count = 0;
  }
}

/*
Handles the completion of the pump operation, displaying appropriate messages based on the error code received.
* Error codes:
* user interruption: err = 1
* tank full: err = 2
* high ampere: err = 3
* low ampere: err = 4
*/
void handlePumpCompletion(byte code)
{

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 10);
  display.print("Msg: ");
  display.print(errorCodeMessage[code - 1]);

  display.setCursor(0, 25);
  display.print("Time Taken: ");
  time_t totalRuntime = pumpStartTime == 0 ? 0 : time(NULL) - pumpStartTime;
  display.print(formatElapsedTime(totalRuntime));
  display.display();
  if (code == ALERT_TANK_FULL && activeAutoRunPeriod >= 1 && activeAutoRunPeriod <= 3)
  {
    doneForToday |= (1 << (activeAutoRunPeriod - 1));
    pref.begin("database", false);
    pref.putUChar("doneToday", doneForToday);
    pref.end();
    activeAutoRunPeriod = 0;
  }

  if (useWifi)
  {
    endTime = currTime;
    if (code == ALERT_TANK_FULL)
    {
      percEnd = 100;
    }
    else
      percEnd = tankLevelPerc();
    if (countAmp > 0)
      sumAmp /= countAmp;

    if (totalRuntime < 180) // don't log if pump ran less than 3 minutes
      yield();
    else
      pumpLog(errorCodeMessage[code - 1]);
  }
  countAmp = 0; // Reset ampere sample count
  sumAmp = 0;   // Reset ampere sum
  resetTimer();

  pixels.setBrightness(250);
  if (code == 2)
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  else
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));

  pixels.show();

  // Non-blocking 1-minute timer for auto return to home screen
  uint32_t messageStartTime = millis();
  const uint32_t messageTimeout = 60000; // 60 seconds in milliseconds
  bool buttonPressed = false;

  while (true)
  {
    uint32_t elapsedTime = millis() - messageStartTime;
    uint32_t remainingSeconds = (messageTimeout - elapsedTime) / 1000;

    // Check if 60 seconds have elapsed - auto exit
    if (elapsedTime >= messageTimeout)
    {
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      break;
    }

    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
    display.setTextColor(SH110X_WHITE);

    // Display remaining time
    // display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(57, 56);
    display.fillRect(47, 55, 30, 10, 0);
    display.print(remainingSeconds);
    display.setTextColor(SH110X_WHITE);

    display.display();
    if (code == 2)
    {
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
    }
    else
    {
      digitalWrite(BUZZER_PIN, HIGH);
    }

    // Non-blocking button check
    if (digitalRead(BUTTON) == 1)
    {

      buttonPressed = true;
      blinkOrange(0, 150, 0);
      delay(250);
    }

    if (buttonPressed && digitalRead(BUTTON) == 0)
    {
      // Button was released - exit immediately
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      break;
    }

    delay(50); // Small delay to prevent overwhelming the CPU
  }
}

// Displays instructions on the OLED for connecting to the WiFi AP created by the device, providing the SSID and password for access.
void wifiManagerInfoPrint()
{
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(1, 0);
  display.println("WIFI MANAGER");
  display.drawLine(0, 8, 127, 8, 1);
  display.setCursor(0, 10);
  display.print("Turn ON WiFi on your phone/laptop.");
  display.setCursor(0, 30);
  display.print("Connect: ");
  display.print("WIFI_MANAGER");
  display.setCursor(0, 42);
  display.print("Password: ");
  display.print("WIFImanager");
  display.display();
}

// Displays instructions on the OLED when a device connects to the WiFi AP, guiding the user to access the configuration page and enter their WiFi credentials.
void WiFiEvent(WiFiEvent_t event)
{
  if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED)
  {
    display.clearDisplay();
    display.setCursor(1, 0);
    display.println("WIFI MANAGER");
    display.drawLine(0, 8, 127, 8, 1);
    display.setCursor(0, 10);
    display.println("On browser, go to");
    display.setCursor(0, 21);
    display.print("192.168.4.1/wifi");
    display.setCursor(0, 31);
    display.print("Enter the Wifi");
    display.setCursor(0, 38);
    display.print("credentials of 2.4Ghz");
    display.print("network. Then press  \"Submit\". And wait.");
    display.display();
  }
}

/*
Logs the pump operation details to a server if WiFi is connected. The log includes the date and time, start and end times, percentage of tank filled at the beginning and end, elapsed time, average amperage, and any error messages.
*/
void pumpLog(String errM)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    // Calculate elapsed time since pump started
    time_t elapsedTime = time(NULL) - pumpStartTime;
    String temp = formatElapsedTime(elapsedTime);

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    String msg = dateAndTime + "#" + startTime + "#" + endTime + "#" + String(percBegin) + "#" + String(percEnd) + "#" + temp + "#" + sumAmp + "#" + errM;

    // If you need an HTTP request with a content type: application/json, use the following:
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apiKey + "&data=" + msg;
    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString(); // Get the response to the request
    Serial.println(response);

    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

/**
 * @brief Checks whether the current RTC time is inside the configured auto-run window.
 *
 * The schedule values must be provided in 24-hour HHMM integer format.
 * For example:
 * - 1230 means 12:30 PM
 * - 1835 means 6:35 PM
 * - 45 means 00:45 AM
 *
 * The function reads the already-updated global RTC fields `timeHour` and
 * `timeMinute`, converts them to the same HHMM format, and compares that
 * value against the supplied start and stop times.
 *
 * Two schedule shapes are supported:
 * - Same-day window: `offTime > onTime`
 *   Example: 1230 to 1330 matches times after 12:30 and before 13:30.
 *
 * - Overnight window: `offTime <= onTime`
 *   Example: 2230 to 0630 matches times after 22:30 or before 06:30.
 *
 * Boundary behavior is strict: the exact `onTime` and exact `offTime` values
 * return false. A time must be greater than `onTime` and less than `offTime`
 * for same-day windows, or fall on either side of the midnight-spanning range
 * for overnight windows.
 *
 * @param onTime Start time of the schedule window in 24-hour HHMM format.
 * @param offTime End time of the schedule window in 24-hour HHMM format.
 * @return true when the current RTC time is inside the schedule window.
 * @return false when the current RTC time is outside the schedule window or
 *         exactly equal to either boundary.
 */
bool checkTimeFor(int onTime, int offTime)
{
  int h = timeHour;
  int m = timeMinute;

  int timeString = h * 100 + m; // if h=12 and m=23 then 12*100 + 23 = 1223 hours

  if (offTime > onTime) // when off timing is greater than on timing
  {
    if ((timeString > onTime) && (timeString < offTime))
    {
      return true;
    }
  }
  else
  {
    if (((timeString > onTime) && (timeString > offTime)) || ((timeString < onTime) && (timeString < offTime)))
    {
      return true;
    }
  }
  return false;
}
