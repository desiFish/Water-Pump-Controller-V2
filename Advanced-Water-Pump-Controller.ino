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

// RGB LED (2812B)
#include <Adafruit_NeoPixel.h>

// For SCT013 Current Sensor
#include "EmonLib.h"

// Google Sheet Logging
#include <HTTPClient.h>

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

/*5 is 5 seconds, you can assign any time value you wish.
 This is given because it takes a while for the current consumption to get stable.
 And there are all sort of current and voltage spikes just after the pump is ON
 Giving it few seconds should stabilize the reading.*/
#define WAIT_AFTER_PUMP_ON 5

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
float liveAmp, avgAmp;
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
byte timeHour, timeMinute, timerHour = 0, timerMinute = 0, timerSecond = 0, timerCount = 0;
int onTime = 1230, offTime = 1330, lastDay;
bool doneForToday, autoRun;
String dateAndTime, onlyTime;
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

TaskHandle_t loop2Code;

// Elegant OTA related task
void onOTAStart()
{
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
}

// forward declaration
void drawTankLevel(byte);
void blinkOrange(byte, byte, int = 50);
void autoTimeUpdate(bool = true);
void pumpRunSequence(bool = false);

/**
 * @brief Initializes the pump controller system
 *
 * This function sets up all necessary hardware components, loads settings,
 * and prepares the system for operation.
 */
void setup(void)
{
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  TURN_OFF_RELAY;
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  pixels.begin();
  pixels.setBrightness(20);
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
  if (!pref.isKey("doneForToday"))
    pref.putBool("doneForToday", false);
  if (!pref.isKey("lastDay"))
    pref.putInt("lastDay", 0);
  if (!pref.isKey("autoRun"))
    pref.putBool("autoRun", false);

  tankLow = pref.getInt("tankLow", 0);
  tankFull = pref.getInt("tankFull", 0);
  ampLow = pref.getFloat("ampLow", 0);
  ampMax = pref.getFloat("ampMax", 0);
  useUltrasonic = pref.getBool("useUltrasonic", false);
  useSensors = pref.getBool("useSensors", false);
  useFloat = pref.getBool("useFloat", false);
  useWifi = pref.getBool("useWifi", true);
  apiKey = pref.getString("apiKey", "");
  doneForToday = pref.getBool("doneForToday", false);
  lastDay = pref.getInt("lastDay", 0);
  autoRun = pref.getBool("autoRun", false);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(34, 10);
    display.println("RTC FAILED");
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
    display.setTextColor(SH110X_WHITE);
    display.display();

    byte count = 0;
    while (true)
    {
      if (digitalRead(BUTTON) == 1)
      {
        while (digitalRead(BUTTON) == 1)
        {
          delay(150);
          count++;
        }
        if (count >= 1)
          break;
      }
    }
  }
  if (autoRun)
  {
    DateTime now = rtc.now();
    if (lastDay != now.day())
    { // if today is a different day, reset "doneForToday"
      lastDay = now.day();
      pref.putInt("lastDay", lastDay);
      pref.putBool("doneForToday", false);
      doneForToday = false;
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
                { request->send(200, "text/plain", "Hi! Please add "
                                                   "/update"
                                                   " on the above address."); });

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
      autoTimeUpdate(false);
    }
  }

  pref.end();
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  uSonicSerial.begin(uSonic_BAUD, SERIAL_8N1, UltraRX, UltraTx);

  emon1.current(CURRENT_SENSOR_PIN, 27); // Current: input pin, calibration.
  analogReadResolution(10);              // read resolution (10=10 bits)

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
  for (;;)
  {
    if (useSensors)
      liveAmp = readAmpere();
    delay(10);

    if (useFloat)
      floatSensor = readFloat(); // reads float sensor value and updates it
    delay(10);

    if (useUltrasonic)
      liveTankLevel = readUltrasonic();
    delay(10);

    if (isPumpRunning)
    {
      raiseAlert = intelligentMonitoring();
      Serial.print("PUMP RUN ERRORCODE");
      Serial.println(": " + String(raiseAlert));
    }
    delay(10);

    DateTime now = rtc.now();
    timeHour = now.hour();
    timeMinute = now.minute();
    dateAndTime = now.timestamp(DateTime::TIMESTAMP_FULL);
    onlyTime = now.timestamp(DateTime::TIMESTAMP_TIME);

    if (isPumpRunning)
    {
      byte sec = now.second();
      if (timerCount != sec)
      {
        timerCount = sec;
        timerSecond++;
        if (timerSecond == 30) // every 30 seconds, average the current
        {
          avgAmp += liveAmp;
          countAmp++;
        }

        if (timerSecond > 59)
        {
          timerSecond = 0;
          timerMinute++;
          if (timerMinute > 59)
          {
            timerMinute = 0;
            timerHour++;
            if (timerHour > 23)
              timerHour = 0;
          }
        }
      }
    }

    if (autoRun && !doneForToday && !isPumpRunning)
    {
      bool flag = checkTimeFor(onTime, offTime);
      if (flag)
      {
        raiseAlert = 99; // special case for automatic pump start
        doneForToday = true;
      }
    }

    delay(10);
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

  if (raiseAlert > 1 && raiseAlert < 5)
  {
    errorMsg(raiseAlert, true);
    raiseAlert = 0;
  }
  else if (raiseAlert == 99)
    runPumpAuto();

  if (!updateInProgress)
  {
    if (resetFlag) // after resetting (set in menu options; reset), esp32 will restart
    {
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
      display.print(onlyTime);

      display.setCursor(60, 1);
      if (isPumpRunning)
      {
        display.println("PUMP: ON");
        pixels.setPixelColor(0, pixels.Color(128, 0, 128));
        pixels.show();
      }
      else if (!doneForToday)
      {
        display.println("PUMP: OFF");
        pixels.setPixelColor(0, pixels.Color(255, 255, 255));
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
  if (digitalRead(BUTTON) == 1)
  {
    while (digitalRead(BUTTON) == 1)
    {
      count++;
      if (count >= 1 && count <= 30)
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

    pixels.setBrightness(20);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();

    if (count >= 1 && count <= 30)
    {
      pixels.setPixelColor(0, pixels.Color(255, 255, 0));
      pixels.show();
      pumpRunSequence();
    }
    else
      menu();
  }
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
    if (isPumpRunning)
    {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(30, 10);
      display.println("STOP PUMP?");
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

      if (digitalRead(BUTTON) == 1)
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

        pixels.setBrightness(20);
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
            break;
          else if (option == 2)
          {
            if (isPumpRunning)
            { // check if pump is running
              isPumpRunning = false;
              TURN_OFF_RELAY;
              delay(200);
              if (useWifi)
              {
                endTime = onlyTime;
                percEnd = tankLevelPerc();
                pumpLog(errorCodeMessage[0]);
              }
              timerReset();
            }
            break;
          }
        }
      }
      count = 0;
      display.display();
    }
    else
    {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(30, 10);
      display.println("START PUMP?");
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

      if (flag || digitalRead(BUTTON) == 1)
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

        if (flag)
        { // for running in auto mode
          count = 50;
          option = 2;
        }

        pixels.setBrightness(20);
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
          else if (option == 2)
          { // YES
            byte errorCode = intelligentMonitoring();
            if (errorCode == 0 || errorCode == 1) // check if there is any error
            {
              TURN_ON_RELAY;
              delay(300);
              if (useWifi)
              {
                startTime = onlyTime;
                percBegin = tankLevelPerc();
              }
              pixels.setPixelColor(0, pixels.Color(0, 255, 255));
              pixels.show();
              pumpOnDelay();
              holdData = 0;
              countAmp = 0;
              avgAmp = 0;
              isPumpRunning = true;
              break;
            }
            else
            {
              errorMsg(errorCode, false); // raise error but do not log
              break;
            }
          }
        }
      }
      count = 0;
      display.display();
    }
  }
}

/**
 * @brief Monitors system parameters and returns error status
 *
 * @return byte Error code (0: No error, 1: Tank not full, 2: Tank full, 3: High ampere, 4: Low ampere)
 *
 * This function checks various sensor readings and system states to ensure safe operation.
 */
byte intelligentMonitoring()
{
  byte err = 0;

  if (isPumpRunning) // PUMP ON OFF BACKUP CONTROL
    TURN_ON_RELAY;
  else
    TURN_OFF_RELAY;

  if (useFloat)
    if (!floatSensor) // tank not full
      err = 1;

  if (useFloat)
  {
    if (floatSensor) // tank full
    {
      err = 2;
      TURN_OFF_RELAY;
      delay(200);
      isPumpRunning = false;
    }
  }

  if (useSensors && isPumpRunning)
  {
    if (liveAmp > ampMax)
    {
      err = 3; // RUN CONDITION WHERE PUMP DRAWS MORE CURRENT
      TURN_OFF_RELAY;
      delay(200);
      isPumpRunning = false;
    }

    if (liveAmp < ampLow)
    {
      err = 4; // RUN CONDITION WHERE PUMP DRAWS LESS CURRENT
      TURN_OFF_RELAY;
      delay(200);
      isPumpRunning = false;
    }

    if (liveAmp > 1.5) // verifies if something is drawing current or not
      isPumpRunning = true;
    else
      isPumpRunning = false;
  }
  return err;
}

void timerReset()
{
  timerMinute = 0;
  timerSecond = 0;
  timerHour = 0;
  timerCount = 0;
}

void pumpOnDelay()
{
  byte i = WAIT_AFTER_PUMP_ON;
  while (i > 0)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(32, 10);
    display.println("PLEASE WAIT");
    display.setCursor(62, 35);
    display.print(i);
    display.display();
    delay(1000);
    yield();
    i--;
  }
  return;
}

void runPumpAuto()
{
  raiseAlert = 0;
  byte i = 10;
  while (i > 0)
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
    display.print(i);
    display.display();
    pixels.setBrightness(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    yield();
    i--;
    bool count = false;
    while (digitalRead(BUTTON) == 1)
    {
      blinkOrange(0, 150, 0);
      delay(100);
      count = true;
    }
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    if (count)
      return;
  }
  pumpRunSequence(true);
}

/**
 * @brief Reads current value from the ampere sensor
 *
 * @return double Current reading in amperes
 */
double readAmpere()
{
  double Irms = emon1.calcIrms(1480); // Calculate Irms only
  return Irms;
}

/**
 * @brief Reads distance value from the ultrasonic sensor
 *
 * @return int Distance reading in centimeters
 */
int readUltrasonic()
{
  static int x;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1)
  {
    previousMillis1 = currentMillis;
    uSonicSerial.write(0x01);
    delay(10);
    if (uSonicSerial.available())
    {
      // Serial.println(uSonicSerial.readString());
      delay(100);
      x = ((uSonicSerial.readString()).substring(4, 8)).toInt();
      x = x / 10;
      Serial.println(String(x) + " cm");
    }
  }
  liveTankLevel = x;
  return liveTankLevel;
}

/**
 * @brief Reads the state of the float sensor
 *
 * @return bool True if water level is high, false otherwise
 */
bool readFloat()
{
  static bool tempFloatVal;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis2 >= interval2)
  {
    previousMillis2 = currentMillis;
    if (analogRead(FLOAT_SENSOR) > 900) // FULL/UP
      tempFloatVal = false;
    else // NOT FULL/DOWN
      tempFloatVal = true;
  }
  floatSensor = tempFloatVal;
  return floatSensor;
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
    display.print(timerHour < 10 ? "0" + String(timerHour) : String(timerHour)); // if hour or minute is less than 10 put a 0 before it
    display.print(":");
    display.print(timerMinute < 10 ? "0" + String(timerMinute) : String(timerMinute));
    display.print(":");
    display.print(timerSecond < 10 ? "0" + String(timerSecond) : String(timerSecond));
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

/*main container function with 1-button system, 1 short click for changing
menu items, 1 long click for selecting the highlighted option
(same for throughout the program)
*/
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

    if (option > 3)
    {
      display.setCursor(3, 13);
      display.print("4. Reset");
      display.setCursor(3, 27);
      display.print("5. Exit");
    }
    else
    {
      display.setCursor(3, 13);
      display.print("1. Sys Watcher");
      display.setCursor(3, 27);
      display.print("2. Data Limits");
      display.setCursor(3, 41);
      display.print("3. Configurations");
      display.setCursor(3, 55);
      display.print("4. Reset");
    }

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
    case 4:
      display.drawRect(0, 10, 127, 13, 1);
      break;
    case 5:
      display.drawRect(0, 24, 127, 13, 1);
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

      pixels.setBrightness(20);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 6)
      {
        option++;
        if (option > 5)
          option = 1;
      }
      else
      {
        if (option == 1)
          sysWatcher();
        else if (option == 2)
          dataLimit();
        else if (option == 3)
          configurations();
        else if (option == 4)
          totalReset();
        else if (option == 5)
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
  if (times == 0)
  {
    pixels.setBrightness(brightValue);
    pixels.setPixelColor(0, pixels.Color(255, 165, 0));
    pixels.show();
  }
  else
  {
    int i = 0;
    while (i < times)
    {
      pixels.setBrightness(brightValue);
      pixels.setPixelColor(0, pixels.Color(255, 165, 0));
      pixels.show();
      delay(blinkDuration);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      delay(blinkDuration);
      i++;
    }
  }
}

/**
 * @brief Manages the data limit settings menu
 *
 * This function provides a user interface for setting various data limits
 * such as ultrasonic sensor and current sensor thresholds.
 * It uses a single-button navigation system.
 */
void dataLimit()
{
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Set Limits");
    display.drawLine(0, 8, 127, 8, 1);

    display.setCursor(3, 13);
    display.print("1. Ultrasonic Sensor");
    display.setCursor(3, 27);
    display.print("2. Current Sensor");
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
        delay(50);
      }

      pixels.setBrightness(20);
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
          ultraSonicValues();
        else if (option == 2)
          ampereValues();
        else if (option == 3)
          return;
      }
    }
    count = 0;
  }
}

/**
 * @brief Configures ultrasonic sensor values
 *
 * This function allows the user to set or adjust the ultrasonic sensor values
 * for tank level measurements. It provides options for using live values or
 * manually entering values for empty and full tank levels.
 *
 * @note This function uses persistent storage to save the configured values.
 */
void ultraSonicValues()
{
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Ultra-Sonic Sensor");
    display.drawLine(0, 8, 127, 8, 1);

    display.setCursor(3, 13);
    display.print("1. Use live values");
    display.setCursor(3, 27);
    display.print("2. Use manual values");
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
        delay(50);
      }

      pixels.setBrightness(20);
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
        {
          byte count = 0, option = 1;
          while (true)
          {
            int newValue = liveTankLevel; // fetch live ultra-sonic values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Ultra-Sonic: EMPTY");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(tankLow) + " cm");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " cm");
            // buttons
            if (option == 1)
            {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else
            {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

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

              pixels.setBrightness(20);
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
                  tankLow = newValue; // save tankLow value in preference
                  pref.putInt("tankLow", tankLow);
                  break;
                }
                else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(300);

          while (true)
          {
            int newValue = liveTankLevel; // fetch live ultra-sonic values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Ultra-Sonic: FULL");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(tankFull) + " cm");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " cm");
            // buttons
            if (option == 1)
            {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else
            {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

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

              pixels.setBrightness(20);
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
                  tankFull = newValue; // save tankFull value in preference
                  pref.putInt("tankFull", tankFull);
                  break;
                }
                else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2)
        {
          byte count = 0, option = 1;
          int newValue = tankLow; // old value
          while (true)
          {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Ultra-Sonic: EMPTY");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(tankLow) + " cm");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1)
            {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 2)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 3)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 4)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1)
            {
              while (digitalRead(BUTTON) == 1)
              {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3)
                {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2)
              {
                option++;
                if (option > 4)
                  option = 1;
              }
              else
              {
                if (option == 1)
                {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                }
                else if (option == 2)
                {
                  newValue++;
                  if (newValue >= MAX_ULTRASONIC_VALUE)
                    newValue = MAX_ULTRASONIC_VALUE;
                }
                else if (option == 3)
                {
                  tankLow = newValue; // save tankLow value in preference
                  pref.putInt("tankLow", tankLow);
                  break;
                }
                else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          pixels.setBrightness(20);
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();

          display.clearDisplay();
          display.display();
          delay(300);

          count = 0, option = 1;
          newValue = tankFull; // old value
          while (true)
          {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Ultra-Sonic: FULL");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(tankFull) + " cm");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1)
            {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 2)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 3)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 4)
            {
              display.print("- ");
              display.print(String(newValue) + " cm ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1)
            {
              while (digitalRead(BUTTON) == 1)
              {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3)
                {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2)
              {
                option++;
                if (option > 4)
                  option = 1;
              }
              else
              {
                if (option == 1)
                {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                }
                else if (option == 2)
                  newValue++;
                else if (option == 3)
                {
                  tankFull = newValue; // save tankFull value in preference
                  pref.putInt("tankFull", tankFull);
                  break;
                }
                else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          pixels.setBrightness(20);
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();
        }
        else if (option == 3)
          return;
      }
    }
    count = 0;
  }
  pref.end();
}

// sets values for ampLow, ampMax, wattLow, wattMax using live values from sensor
void ampereValues()
{
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Ampere Sensor");
    display.drawLine(0, 8, 127, 8, 1);

    display.setCursor(3, 13);
    display.print("1. Use live values");
    display.setCursor(3, 27);
    display.print("2. Use manual values");
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
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      pixels.setBrightness(20);
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
        {
          byte count = 0, option = 1;
          while (true)
          {
            float newValue = liveAmp; // fetch live ampere sensor values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Current: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(ampLow) + " A");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " A");
            // buttons
            if (option == 1)
            {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else
            {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

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

              pixels.setBrightness(20);
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
                  ampLow = newValue; // save ampLow value in preference
                  pref.putFloat("ampLow", ampLow);
                  break;
                }
                else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(300);

          while (true)
          {
            float newValue = liveAmp; // fetch live Ampere values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Current: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(ampMax) + " A");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " A");
            // buttons
            if (option == 1)
            {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else
            {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

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

              pixels.setBrightness(20);
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
                  ampMax = newValue; // save ampMax value in preference
                  pref.putFloat("ampMax", ampMax);
                  break;
                }
                else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2)
        {
          byte count = 0, option = 1;
          float newValue = ampLow; // old value
          while (true)
          {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Current: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(ampLow) + " A");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1)
            {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 2)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 3)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 4)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1)
            {
              while (digitalRead(BUTTON) == 1)
              {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3)
                {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2)
              {
                option++;
                if (option > 4)
                  option = 1;
              }
              else
              {
                if (option == 1)
                {
                  newValue = newValue - 0.1;
                  if (newValue < 0)
                    newValue = 0;
                }
                else if (option == 2)
                  newValue = newValue + 0.1;
                else if (option == 3)
                {
                  ampLow = newValue; // save ampLow value in preference
                  pref.putFloat("ampLow", ampLow);
                  break;
                }
                else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          pixels.setBrightness(20);
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();

          display.clearDisplay();
          display.display();
          delay(300);

          count = 0, option = 1;
          newValue = ampMax; // old value
          while (true)
          {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Current: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(ampMax) + " A");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1)
            {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 2)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 3)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            }
            else if (option == 4)
            {
              display.print("- ");
              display.print(String(newValue) + " A ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1)
            {
              while (digitalRead(BUTTON) == 1)
              {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3)
                {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2)
              {
                option++;
                if (option > 4)
                  option = 1;
              }
              else
              {
                if (option == 1)
                {
                  newValue = newValue - 0.1;
                  if (newValue < 0)
                    newValue = 0;
                }
                else if (option == 2)
                  newValue = newValue + 0.1;
                else if (option == 3)
                {
                  ampMax = newValue; // save ampMax value in preference
                  pref.putFloat("ampMax", ampMax);
                  break;
                }
                else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          pixels.setBrightness(20);
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();
        }
        else if (option == 3)
          return;
      }
    }

    count = 0;
  }
  pref.end();
}

void configurations()
{
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Configurations");
    display.drawLine(0, 8, 127, 8, 1);

    if (option > 3)
    {
      display.setCursor(3, 13);
      display.print("4. Wifi");
      display.setCursor(3, 27);
      display.print("5. Autorun");
      display.setCursor(3, 41);
      display.print("6. Exit");
    }
    else
    {
      display.setCursor(3, 13);
      display.print("1. Ultrasonic");
      display.setCursor(3, 27);
      display.print("2. Time Update");
      display.setCursor(3, 41);
      display.print("3. Other Sensors");
      display.setCursor(3, 55);
      display.print("4. Wifi");
    }
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
    case 4:
      display.drawRect(0, 10, 127, 13, 1);
      break;
    case 5:
      display.drawRect(0, 24, 127, 13, 1);
      break;
    case 6:
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
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      pixels.setBrightness(20);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();

      if (count >= 1 && count <= 6)
      {
        option++;
        if (option > 6)
          option = 1;
      }
      else
      {
        if (option == 1)
          break;
        else if (option == 2)
          configTime();
        else if (option == 3)
          configSensors(); // Bypass Sensors (Current and Float)
        else if (option == 4)
          break;
        else if (option == 5)
          break;
        else if (option == 6)
          return;
      }
    }
    count = 0;
  }
  // for ultrasonic and wifi and rest in separate function
  if (option == 1)
  {
    if (useUltrasonic)
      option = 1;
    else
      option = 2;
    while (true)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Ultrasonic");
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

        pixels.setBrightness(20);
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
            useUltrasonic = true;
            pref.putBool("useUltrasonic", true);
            break;
          }
          else if (option == 2)
          {
            useUltrasonic = false;
            pref.putBool("useUltrasonic", false);
            break;
          }
        }
      }
      count = 0;
    }
  }
  else if (option == 4)
  {
    if (useWifi)
      option = 1;
    else
      option = 2;
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

        pixels.setBrightness(20);
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
  }
  else if (option == 5)
  {
    if (autoRun)
      option = 1;
    else
      option = 2;
    while (true)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("AUTORUN");
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

        pixels.setBrightness(20);
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
            autoRun = true;
            pref.putBool("autoRun", true);
            break;
          }
          else if (option == 2)
          {
            autoRun = false;
            pref.putBool("autoRun", false);
            break;
          }
        }
      }
      count = 0;
    }
  }
  display.clearDisplay();
  pref.end();
}

void configSensors()
{
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Configurations");
    display.drawLine(0, 8, 127, 8, 1);

    display.setCursor(3, 13);
    display.print("1. Current Sensor");
    display.setCursor(3, 27);
    display.print("2. Float Sensor");
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
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      pixels.setBrightness(20);
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
          break;
        else if (option == 2)
          break;
        else if (option == 3)
          return;
      }
    }
    count = 0;
  }
  if (option == 1)
  {
    if (useSensors)
      option = 1;
    else
      option = 2;
    while (true)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Current Sensor");
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

        pixels.setBrightness(20);
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
            useSensors = true;
            pref.putBool("useSensors", true);
            break;
          }
          else if (option == 2)
          {
            useSensors = false;
            pref.putBool("useSensors", false);
            break;
          }
        }
      }
      count = 0;
      display.display();
    }
  }
  else if (option == 2)
  {
    if (useFloat)
      option = 1;
    else
      option = 2;
    while (true)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Float Sensor");
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

        pixels.setBrightness(20);
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
            useFloat = true;
            pref.putBool("useFloat", true);
            break;
          }
          else if (option == 2)
          {
            useFloat = false;
            pref.putBool("useFloat", false);
            break;
          }
        }
      }
      count = 0;
      display.display();
    }
  }
  display.clearDisplay();
  pref.end();
}

// Manual time set not implemented yet
void configTime()
{
  byte count = 0, option = 1;
  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Time Update");
    display.drawLine(0, 8, 127, 8, 1);

    display.setCursor(3, 13);
    display.print("1. Auto (Internet)");
    display.setCursor(3, 27);
    display.print("2. Manual");
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
          blinkOrange(1, 20);
        }
        else
        {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      pixels.setBrightness(20);
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
          autoTimeUpdate();
        else if (option == 2)
          break; // implement this
        else if (option == 3)
          return;
      }
    }
    count = 0;
  }
  display.clearDisplay();
}

// Test this for disconnection error
void autoTimeUpdate(bool temp)
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
    }
    if (temp)
    {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(44, 10);
      display.println("DONE");
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.setCursor(50, 41);
      display.fillRect(47, 40, 29, 10, 1);
      display.print("OKAY");
      display.setTextColor(SH110X_WHITE);
      display.display();

      byte count = 0;
      while (1)
      {
        if (digitalRead(BUTTON) == 1)
        {
          while (digitalRead(BUTTON) == 1)
          {
            count++;
            if (count >= 1)
            {
              blinkOrange(1, 20);
            }
            delay(50);
          }

          pixels.setBrightness(20);
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();

          if (count >= 1)
            break;
        }
      }
    }
  }
  else
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(14, 10);
    display.println("WIFI NOT CONNECTED");
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
    display.setTextColor(SH110X_WHITE);
    display.display();

    byte count = 0;
    while (1)
    {
      if (digitalRead(BUTTON) == 1)
      {
        while (digitalRead(BUTTON) == 1)
        {
          count++;
          if (count >= 1)
            blinkOrange(1, 20);
          delay(50);
        }

        pixels.setBrightness(20);
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();

        if (count >= 1)
          break;
      }
    }
  }
  return;
}

void totalReset()
{
  byte count = 0, option = 1;
  pixels.setBrightness(150);
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
  while (true)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 10);
    display.println("Reset Everything?");
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

      pixels.setBrightness(20);
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
          pref.putInt("tankLow", 0);
          pref.putInt("tankFull", 0);
          pref.putFloat("ampLow", 0);
          pref.putFloat("ampMax", 0);
          pref.putBool("useUltrasonic", false);
          pref.putBool("useSensors", false);
          pref.putBool("useFloat", false);
          pref.putBool("useWifi", true);

          pref.end();
          resetFlag = true;
        }
      }
    }
    count = 0;
  }
}

/*prints various error messages on screen
\n tank full: err = 2
\n high ampere: err = 3
\n low ampere: err = 4
*/
void errorMsg(byte code, bool sheetLogger)
{
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 10);
  display.print("Msg: ");
  display.print(errorCodeMessage[code - 1]);
  // Print how much time it took
  display.setCursor(0, 25);
  display.print("Time Taken: ");
  display.print(timerHour < 10 ? "0" + String(timerHour) : String(timerHour)); // if hour or minute is less than 10 put a 0 before it
  display.print(":");
  display.print(timerMinute < 10 ? "0" + String(timerMinute) : String(timerMinute));
  display.print(":");
  display.print(timerSecond < 10 ? "0" + String(timerSecond) : String(timerSecond));
  display.display();
  if (useWifi && sheetLogger)
  {
    endTime = onlyTime;
    if (code == 2)
    {
      percEnd = 100;
      pref.begin("database", false);
      pref.putBool("doneForToday", true); // makes sure that it is done for today, hence no more auto run
      pref.end();
    }
    else
      percEnd = tankLevelPerc();
    avgAmp += liveAmp;
    countAmp++;
    avgAmp /= countAmp;
    if (timerHour == 0 && timerMinute == 0 && timerSecond < 15) // don't log if total run time is less than 15 secs
      yield();
    else
      pumpLog(errorCodeMessage[code - 1]);
  }
  timerReset();
  pixels.setBrightness(250);
  if (code == 2)
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  else
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));

  pixels.show();

  while (true)
  {
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
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

    bool count = false;

    while (digitalRead(BUTTON) == 1)
    {
      blinkOrange(0, 150, 0);
      delay(100);
      count = true;
    }

    if (count)
    {
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      return;
    }
  }
}

/*
Prints various system related data
!! Add Current Sensor
*/
void sysWatcher()
{
  byte data = 0, option = 0, count = 0;
  while (1)
  {
    if (data == 0)
    {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("CPU Speed: " + String(getCpuFrequencyMhz()) + "Mhz");
      display.setCursor(0, 10);
      display.print("XTAL Freq: " + String(getXtalFrequencyMhz()) + "Mhz");
      display.setCursor(0, 20);
      display.print("APB Bus: " + String(getApbFrequency()) + "hz");
      display.setCursor(0, 30);
      int8_t temp = WiFi.RSSI();
      display.print("WIFI RSSI: " + String(temp));
      if (temp >= -50)
        display.print(" (WOW)");
      else if (temp > -60 && temp < -50)
        display.print(" (GOOD)");
      else if (temp > -70 && temp <= -60)
        display.print(" (FAIR)");
      else if (temp <= -70)
        display.print(" (WEAK)");
      display.setCursor(0, 40);
      display.print("IP: ");
      display.print(WiFi.localIP());
      display.drawCircle(70, 50, 2, 1);
      display.fillCircle(59, 50, 2, 1);
    }
    else if (data == 1)
    {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Float: " + String(floatSensor));
      if (useUltrasonic)
      {
        display.setCursor(0, 10);
        display.println("U.S.: " + String(liveTankLevel) + " cm");
      }
      display.fillCircle(70, 50, 2, 1);
      display.drawCircle(59, 50, 2, 1);
    }

    if (option == 0)
    {
      display.setCursor(32, 56);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(28, 55, 29, 10, 1);
      display.print("BACK");
      display.setTextColor(SH110X_WHITE);
      display.setCursor(75, 56);
      display.print("NEXT");
      display.setTextColor(SH110X_WHITE);
      display.display();
    }
    else if (option == 1)
    {
      display.setCursor(32, 56);
      display.setTextColor(SH110X_WHITE);
      display.print("BACK");
      display.setCursor(75, 56);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(71, 55, 29, 10, 1);
      display.print("NEXT");
      display.setTextColor(SH110X_WHITE);
      display.display();
    }

    if (digitalRead(BUTTON) == 1)
    {
      while (digitalRead(BUTTON) == 1)
      {
        count++;
        if (count >= 1 && count <= 2)
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

      pixels.setBrightness(20);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      if (count >= 1 && count <= 2)
      {
        option++;
        if (option > 1)
          option = 0;
      }
      else
      {
        if (option == 0)
          return;
        else if (option == 1)
        {
          data++;
          if (data > 1)
            data = 0;
        }
      }
    }
    count = 0;
  }
}

// WIFI MANAGER HELPING FUNCTIONS
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

void pumpLog(String errM)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    String temp = String(timerHour) + ":" + String(timerMinute) + ":" + String(timerSecond);

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    String msg = dateAndTime + "#" + startTime + "#" + endTime + "#" + String(percBegin) + "#" + String(percEnd) + "#" + temp + "#" + avgAmp + "#" + errM;

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

// checks if current time is between onTime and offTime, if yes returns true else returns false. 24 Hour mode ONLY
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
