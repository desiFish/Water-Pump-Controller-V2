/*
Copyright Aniket Patra. Give proper attributes. Others (one or more) parties may have their own copyrights of their work.

Hardware:
1. DOIT ESP32 DEVKIT V1
2. 128x64 OLED Display
3. DS1307 RTC
4. SCT Current sensor
5. AJ-SR04 Ultrasonic Sensor(Water Proof)

NOT USING CURRENTLY--> ZMPT101B Voltage Sensor
*/

//For basic ESP32 stuff like wifi, OTA Update and Wifi Manager Server
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

//For Display and I2C
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Fonts/FreeSerif9pt7b.h"

//Data Storage
#include <Preferences.h>

//For ZMPT101B Voltage Sensor
//#include <Filters.h>

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include "RTClib.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

//RGB LED (2812B)
#include <FastLED.h>

//For SCT013 Current Sensor
#include "EmonLib.h"
EnergyMonitor emon1;

//Input Devices
#define BUTTON 15
//#define VOLTAGE_SENSOR 34
#define UltraRX 16  //to TX of sensor
#define UltraTx 17  //to RX of sensor
//SDA 21, SCL 22 used for I2C Devices
#define LED_PIN 4  //for WS2812B RGB LED
#define BUZZER_PIN 5
#define FLOAT_SENSOR 36
#define PUMP_PIN 2             //PUMP RELAY PIN
#define CURRENT_SENSOR_PIN 39  //SCT SENSOR PIN

//Output Devices
#define TURN_ON_RELAY digitalWrite(PUMP_PIN, HIGH)  //update this
#define TURN_OFF_RELAY digitalWrite(PUMP_PIN, LOW)  //update this

/*5 is 5 seconds, you can assign any time value you wish.
 This is given because it takes a while for the current consumption to get stable.
 And there are all sort of current and voltage spikes just after the pump is ON
 Giving it few seconds should resolve it.*/
#define WAIT_AFTER_PUMP_ON 5

#define NUM_LEDS 1
// Define the array of leds
CRGB leds[NUM_LEDS];

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial uSonicSerial(2);
#define uSonic_BAUD 9600
#define MAX_ULTRASONIC_VALUE 400  //400cm or 4meters or 4000 mm max distance read for this model (update accordingly)

Preferences pref;

RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800);  //19800 is offset of India, asia.pool.ntp.org is close to India


//your wifi name and password (used in preference)
String ssid;
String password;

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

//For Display
#define i2c_Address 0x3c  //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Wifi Manager HTML Code
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Wi-Fi Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
html {
  font-family: Arial, Helvetica, sans-serif; 
  display: inline-block; 
  text-align: center;
}

h1 {
  font-size: 1.8rem; 
  color: white;
}

p { 
  font-size: 1.4rem;
}

.topnav { 
  overflow: hidden; 
  background-color: #0A1128;
}

body {  
  margin: 0;
}

.content { 
  padding: 5%;
}

.card-grid { 
  max-width: 800px; 
  margin: 0 auto; 
  display: grid; 
  grid-gap: 2rem; 
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
}

.card { 
  background-color: white; 
  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
}

.card-title { 
  font-size: 1.2rem;
  font-weight: bold;
  color: #034078
}

input[type=submit] {
  border: none;
  color: #FEFCFB;
  background-color: #034078;
  padding: 15px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  width: 100px;
  margin-right: 10px;
  border-radius: 4px;
  transition-duration: 0.4s;
  }

input[type=submit]:hover {
  background-color: #1282A2;
}

input[type=text], input[type=number], select {
  width: 50%;
  padding: 12px 20px;
  margin: 18px;
  display: inline-block;
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}

label {
  font-size: 1.2rem; 
}
.value{
  font-size: 1.2rem;
  color: #1282A2;  
}
.state {
  font-size: 1.2rem;
  color: #1282A2;
}
button {
  border: none;
  color: #FEFCFB;
  padding: 15px 32px;
  text-align: center;
  font-size: 16px;
  width: 100px;
  border-radius: 4px;
  transition-duration: 0.4s;
}
.button-on {
  background-color: #034078;
}
.button-on:hover {
  background-color: #1282A2;
}
.button-off {
  background-color: #858585;
}
.button-off:hover {
  background-color: #252524;
} 
  </style>
</head>
<body>
  <div class="topnav">
    <h1>Wi-Fi Manager</h1>
  </div>
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <form action="/wifi" method="POST">
          <p>
            <label for="ssid">SSID</label>
            <input type="text" id ="ssid" name="ssid"><br>
            <label for="pass">Password</label>
            <input type="text" id ="pass" name="pass"><br>
            <input type ="submit" value ="Submit">
          </p>
        </form>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";

//variables tankLow for storing ultrasonic value for empty tank and tankFull for full level.
int tankLow, tankFull, liveTankLevel;
//variables ampLow for lowest safe level and ampMax for safe ampere max value.
float ampLow, ampMax;
float liveAmp;
//variables wattLow for lowest safe level and wattMax for safe watt max value, since voltage won't be monitored for safety checks.
//float wattLow, wattMax, liveWatt;
//pump status
bool isPumpRunning = false;
//float sensor status
bool floatSensor = false;
//using sensors or not
bool useUltrasonic, useSensors, useFloat;
bool resetFlag = false, updateInProgress = false;
//global error tracking variable, Core 0 updates it
byte raiseError = false;
//variables voltLow for lowest safe level and voltMax for safe voltage max value.
//float liveVoltage, voltLow, voltMax;

//display update frequency
unsigned long previousMillis = 0;  // will store last time it was updated
long interval = 2000;              // interval to wait (milliseconds)

//ultrasonic update frequency
unsigned long previousMillis1 = 0;  // will store last time it was updated
long interval1 = 1000;              // interval to wait (milliseconds)

//ZMPT101B Voltage Sensor data
/*float testFrequency = 50;                   // test signal frequency (Hz)
float windowLength = 40.0 / testFrequency;  // how long to average the signal, for statistics

int Sensor = 0;  //Sensor analog input, here it's A0

float intercept = -0.04;    // to be adjusted based on calibration testing
float slope = 0.0405;       // 0.0405 to be adjusted based on calibration testing
float current_Volts;  // Voltage

unsigned long updatePeriod = 1000;  //Refresh rate
unsigned long previousMillis2 = 0;

RunningStatistics inputStats;  //actual calculation of the RMS requires a load of coding, and this saves us from that
*/

TaskHandle_t loop2Code;
//core 0 debug update frequency
unsigned long previousMillis3 = 0;  // will store last time it was updated
long interval3 = 1000;
//core 1 debug update frequency
unsigned long previousMillis4 = 0;  // will store last time it was updated
long interval4 = 1000;

//Elegant OTA related task
void onOTAStart() {
  // Log when OTA has started
  updateInProgress = true;
  FastLED.setBrightness(200);
  leds[0] = CRGB::Red;
  FastLED.show();
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

void onOTAProgress(size_t current, size_t final) {
  // Log
  if (millis() - ota_progress_millis > 500) {
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

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(2, 10);
    display.println("OTA UPDATE SUCCESSFUL");
    display.display();
  } else {
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

//forward declaration
void drawTankLevel(byte);
void blinkOrange(byte, byte, int = 50);

//runs only once during startup
void setup(void) {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
  leds[0] = CRGB::Red;
  FastLED.show();
  pinMode(BUTTON, INPUT);
  pref.begin("database", false);
  //delay(250);  // wait for the OLED to power up
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
  delay(1000);

  //loading preset values from the memory
  /*wattLow = pref.getInt("wattLow", 0);
  wattMax = pref.getInt("wattMax", 0);
  voltLow = pref.getInt("voltLow", 0);
  voltMax = pref.getInt("voltMax", 0);*/
  bool checkVal = pref.isKey("tankLow");
  if (!checkVal) {
    pref.putInt("tankLow", 0);
  }
  checkVal = pref.isKey("tankFull");
  if (!checkVal) {
    pref.putInt("tankFull", 0);
  }
  checkVal = pref.isKey("ampLow");
  if (!checkVal) {
    pref.putFloat("ampLow", 0);
  }
  checkVal = pref.isKey("ampMax");
  if (!checkVal) {
    pref.putFloat("ampMax", 0);
  }
  checkVal = pref.isKey("useUltrasonic");
  if (!checkVal) {
    pref.putBool("useUltrasonic", 0);
  }
  checkVal = pref.isKey("useSensors");
  if (!checkVal) {
    pref.putBool("useSensors", 0);
  }
  checkVal = pref.isKey("useFloat");
  if (!checkVal) {
    pref.putBool("useFloat", 0);
  }


  tankLow = pref.getInt("tankLow", 0);
  tankFull = pref.getInt("tankFull", 0);
  ampLow = pref.getFloat("ampLow", 0);
  ampMax = pref.getFloat("ampMax", 0);
  useUltrasonic = pref.getBool("useUltrasonic", false);
  useSensors = pref.getBool("useSensors", false);
  useFloat = pref.getBool("useFloat", false);

  //wifi manager
  bool wifiConfigExist = pref.isKey("ssid");
  if (!wifiConfigExist) {
    pref.putString("ssid", "");
    pref.putString("password", "");
  }

  ssid = pref.getString("ssid", "");
  password = pref.getString("password", "");

  if (ssid == "" || password == "") {
    Serial.println("No values saved for ssid or password");
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("WIFI_MANAGER", "WIFImanager");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    wifiManagerInfoPrint();

    // Web Server Root URL
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/html", index_html);
    });

    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        const AsyncWebParameter* p = request->getParam(i);
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
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. Device will now restart.");
      delay(3000);
      ESP.restart();
    });
    server.begin();
    WiFi.onEvent(WiFiEvent);
    while (true)
      ;
  }

  leds[0] = CRGB::Yellow;
  FastLED.show();

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname("Pump Controller");
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
  /*
  count variable stores the status of WiFi connection. 0 means NOT CONNECTED. 1 means CONNECTED
  */
  bool count = 1;
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
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
  if (count) {
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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/plain", "Hi! Please add "
                                       "/update"
                                       " on the above address.");
    });

    ElegantOTA.begin(&server);  // Start ElegantOTA
    // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
    Serial.println("HTTP server started");
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    //while (1) delay(10);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    //display.setFont(NULL);
    display.setCursor(34, 10);
    display.println("RTC FAILED");
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
    display.setTextColor(SH110X_WHITE);
    display.display();

    byte count = 0;
    while (1) {
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          delay(150);
          count++;
        }
        if (count >= 1 && count <= 100)
          break;
      }
    }
  }

  pref.end();
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  uSonicSerial.begin(uSonic_BAUD, SERIAL_8N1, UltraRX, UltraTx);

  emon1.current(CURRENT_SENSOR_PIN, 27);  // Current: input pin, calibration.
  analogReadResolution(10);               //Needed for 12bit resolution

  //inputStats.setWindowSecs(windowLength);

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    loop2,       /* Task function. */
    "loop2Code", /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &loop2Code,  /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  leds[0] = CRGB::Green;
  FastLED.show();
}

/*loop2() runs on Core 0, it is meant for continuously running and updating various
vital sensor data and taking actions based on that. Even if user is operating Menu, it will turn off the pump in background
*/
void loop2(void* pvParameters) {
  for (;;) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis3 >= interval3) {
      // save the last time you blinked the LED
      previousMillis3 = currentMillis;
      Serial.print("This task is running on core ");  //debugging purpose
      Serial.println(xPortGetCoreID());
    }
    /* Sensor = analogRead(VOLTAGE_SENSOR);  // read the analog voltage in value:
    inputStats.input(Sensor);             // log to Stats function
    */
    if (useSensors) {
      //liveVoltage = readVoltage();
      //delay(50);
      liveAmp = readAmpere();
    }
    delay(50);

    if (useFloat)
      floatSensor = readFloat();  // reads float sensor value and updates it

    if (useUltrasonic)
      liveTankLevel = readUltrasonic();

    if (isPumpRunning) {
      raiseError = intelligentMonitoring();
      Serial.println("PUMP RUN ERRORCODE");
      Serial.println("Code: " + String(raiseError));
    }
  }
}

//loop() runs on Core 1, loop performs User Interaction and User Interface
void loop(void) {
  unsigned long currentMillis4 = millis();
  if (currentMillis4 - previousMillis4 >= interval4) {
    // save the last time you blinked the LED
    previousMillis4 = currentMillis4;
    Serial.print("This task is running on core ");  //debugging purpose
    Serial.println(xPortGetCoreID());
  }

  //implement no OTA during pumpIsRunning
  ElegantOTA.loop();

  if (!updateInProgress) {
    if (raiseError != 0 && raiseError != 1) {
      errorMsg(raiseError);
      raiseError = 0;
    }

    if (resetFlag)  //after resetting (set in menu options; reset), esp32 will restart
    {
      leds[0] = CRGB::Red;
      FastLED.show();
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(5, 20);
      display.println("PUMP CONTROLLER WILL");
      display.setCursor(5, 40);
      display.print("RESTART NOW");
      display.display();
      delay(4000);
      ESP.restart();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      DateTime now = rtc.now();

      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);

      display.setCursor(10, 1);
      display.print(now.hour() < 10 ? "0" + String(now.hour()) : String(now.hour()));
      display.print(":");
      display.print(now.minute() < 10 ? "0" + String(now.minute()) : String(now.minute()));

      display.setCursor(60, 1);
      if (isPumpRunning) {
        leds[0] = CRGB::Purple;
        FastLED.show();
        display.println("PUMP: ON");
      } else {
        display.println("PUMP: OFF");
        leds[0] = CRGB::Green;
        FastLED.show();
      }

      if (useUltrasonic) {
        int sonar = liveTankLevel;
        int temp = sonar - tankLow;
        int perc = (temp / (tankFull - tankLow)) * 100;
        drawTankLevel(perc);
      }
      vitals();
      display.display();
    }


    //long press to activate menu

    byte count = 0;
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20, 50);
        } else {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        leds[0] = CRGB::Yellow;
        FastLED.show();
        pumpRunSequence();
      } else {
        menu();
      }
      count = 0;
    }
  }
}

//turns the pump on with safety checks
void pumpRunSequence(void) {
  delay(100);
  byte count = 0, option = 1;
  while (true) {
    if (isPumpRunning) {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(30, 10);
      display.println("STOP PUMP?");
      //buttons
      if (option == 1) {
        display.setCursor(20, 40);
        display.print("YES");
        display.setCursor(90, 40);
        display.fillRect(88, 39, 15, 10, 1);
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print("NO");
        display.setTextColor(SH110X_WHITE);
      } else {
        display.setCursor(20, 40);
        display.fillRect(18, 39, 21, 10, 1);
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print("YES");
        display.setTextColor(SH110X_WHITE);
        display.setCursor(90, 40);
        display.print("NO");
      }

      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1 && count <= 2) {
            blinkOrange(1, 20);
          } else {
            blinkOrange(0, 150);
            delay(100);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1 && count <= 2) {
          option++;
          if (option > 2)
            option = 1;
        } else {
          if (option == 1) {
            leds[0] = CRGB::Green;
            FastLED.show();
            break;
          } else if (option == 2) {

            isPumpRunning = false;
            TURN_OFF_RELAY;
            break;
          }
        }
      }
      count = 0;
      display.display();
    } else {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(30, 10);
      display.println("START PUMP?");
      //buttons
      if (option == 1) {
        display.setCursor(20, 40);
        display.print("YES");
        display.setCursor(90, 40);
        display.fillRect(88, 39, 15, 10, 1);
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print("NO");
        display.setTextColor(SH110X_WHITE);
      } else {
        display.setCursor(20, 40);
        display.fillRect(18, 39, 21, 10, 1);
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        display.print("YES");
        display.setTextColor(SH110X_WHITE);
        display.setCursor(90, 40);
        display.print("NO");
      }

      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1 && count <= 2) {
            blinkOrange(1, 20);
          } else {
            blinkOrange(0, 150);
            delay(100);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1 && count <= 2) {
          option++;
          if (option > 2)
            option = 1;
        } else {
          if (option == 1) {
            leds[0] = CRGB::Green;
            FastLED.show();
            break;
          } else if (option == 2) {
            byte errorCode = intelligentMonitoring();
            if (errorCode == 0 || errorCode == 1)  //check if there is any error
            {
              TURN_ON_RELAY;
              leds[0] = CRGB::Cyan;
              FastLED.show();
              pumpOnDelay();
              isPumpRunning = true;
              break;
            } else {
              leds[0] = CRGB::Red;
              FastLED.show();
              errorMsg(errorCode);
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

/*live monitoring of various sensor details, updating global variables, and returning appropriate error code.
\n no err: err = 0
\n tank not full: err = 1
\n tank full: err = 2
\n high ampere: err = 3
\n high watt: err = 4
\n dry run: err = 5
\n low voltage: err = 6
\n high voltage: err = 7
\n Low Ampere: err = 8
*/
byte intelligentMonitoring() {
  byte err = 0;

  if (isPumpRunning)  //PUMP ON OFF MASTER CONTROL
    TURN_ON_RELAY;
  else
    TURN_OFF_RELAY;

  if (useUltrasonic && !useFloat) {  //if float is diabled
    if (liveTankLevel < tankFull)
      err = 1;
  } else if (useFloat && !useUltrasonic) {  //if ultrasonic is diabled
    if (!floatSensor)                       //update the floatSensor values
      err = 1;
  } else if (useFloat && useUltrasonic) {
    if (!floatSensor || (liveTankLevel < tankFull))  //update the floatSensor values
      err = 1;
  } else err = 1;  //if both the sensors are disabled


  if (useUltrasonic && !useFloat) {  //if float is diabled
    if (liveTankLevel >= tankFull) {
      err = 2;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(200);
    }
  } else if (useFloat && !useUltrasonic) {  //if ultrasonic is diabled
    if (floatSensor)                        //update the floatSensor values
    {
      err = 2;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(200);
    }
  } else if (useFloat && useUltrasonic) {
    if (floatSensor || (liveTankLevel >= tankFull))  //update the floatSensor values
    {
      err = 2;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(200);
    }
  }

  if (useSensors && isPumpRunning) {
    //liveWatt = liveVoltage * liveAmp;

    if (liveAmp > ampMax) {
      err = 3;  //DRY RUN CONDITION WHERE PUMP DRAWS MORE CURRENT
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(500);
    }

    if (liveAmp < ampLow) {
      err = 8;  //DRY RUN CONDITION WHERE PUMP DRAWS MORE CURRENT
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(500);
    }

    /*if (liveWatt > wattMax) {  //check real values for dry running (case 5) and high watt and combine these if necessary
      err = 4;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(500);
    }

    if (liveVoltage < voltLow) {  //UNDER-VOLTAGE
      err = 6;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(500);
    }

    if (liveVoltage > voltMax) {  //OVER-VOLTAGE
      err = 7;
      TURN_OFF_RELAY;
      isPumpRunning = false;
      delay(500);
    }*/

    if (liveAmp > 2.0)  //verifies if something is drawing current or not
      isPumpRunning = true;
    else
      isPumpRunning = false;
  }

  return err;
}

void pumpOnDelay() {
  byte i = WAIT_AFTER_PUMP_ON;
  while (i > 0) {
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
    i--;
  }
  return;
}

/*reads live values from voltage sensor
float readVoltage() {
  if ((unsigned long)(millis() - previousMillis2) >= updatePeriod) {
    previousMillis2 = millis();  // update time every second

    Serial.print("\n");

    current_Volts = intercept + slope * inputStats.sigma();  //Calibartions for offset and amplitude
    current_Volts = current_Volts * (40.3231);               //40.3231 Further calibrations for the amplitude

    Serial.print("\tVoltage: ");
    Serial.println(current_Volts);  //Calculation and Value display is done the rest is if you're using an OLED display
  }
  if (current_Volts < 0)
    current_Volts = 0;
  return current_Volts;
}*/

//reads live values from ampere sensor
double readAmpere() {
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  return Irms;
}

//reads live values from Ultrasonic sensor
int readUltrasonic() {
  int x = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    uSonicSerial.write(0x01);
    delay(10);
    if (uSonicSerial.available()) {
      //Serial.println(uSonicSerial.readString());
      delay(100);
      x = ((uSonicSerial.readString()).substring(4, 8)).toInt();
      x = x / 10;

      Serial.println(String(x) + " cm");
    }
  }
  liveTankLevel = x;
  return liveTankLevel;
}

//reads live values from Float sensor
bool readFloat() {
  if (analogRead(FLOAT_SENSOR) > 900)  //FULL/UP
    floatSensor = false;
  else  //NOT FULL/DOWN
    floatSensor = true;

  return floatSensor;
}

// send data in percent. it prints tank water level in graphical form
void drawTankLevel(byte x = 0) {
  byte temp = x;
  display.drawRoundRect(3, 20, 40, 41, 3, 1);
  display.drawRoundRect(13, 18, 20, 3, 3, 1);

  if (x == 0)
    display.fillRect(5, 57, 36, 0, 1);
  else {
    /*if (x > 99)
      x = 99;*/
    if (x < 3)
      x = 3;
    byte y = x / 3;
    display.fillRect(5, 57, 36, -(y), 1);  //y is max -33

    display.fillRect(14, 27, 19, 9, 1);
    display.setCursor(15, 28);
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(String(temp) + "%");
    display.setTextColor(SH110X_WHITE);
  }
}

//Live value printer function
void vitals() {
  if (useSensors) {
    /*display.setCursor(50, 12);
    display.print("Volt: " + String(liveVoltage) + "V");    
    display.setCursor(50, 22);
    liveWatt = liveAmp * liveVoltage;
    display.print("Watt: " + String(liveAmp * liveVoltage) + " W");*/
    display.setCursor(50, 32);
    display.print("Amp : " + String(liveAmp) + " A");
  }

  if (useFloat) {
    display.setCursor(50, 42);
    if (floatSensor)
      display.print("Float: Full");
    else
      display.print("Float: Not Full");
  }

  if (useUltrasonic) {
    display.setCursor(50, 52);
    display.print("U.S.: " + String(liveTankLevel) + " cm");
  }
}

/*main container function with 1-button system, 1 short click for changing 
menu items, 1 long click for selecting the highlighted option 
(same for throughout the program)
*/
void menu(void) {
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("MENU");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Sys Watcher");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Data Limits");
        display.setCursor(3, 41);
        display.print("3. Configurations");
        display.setCursor(3, 55);
        display.print("4. Reset");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Sys Watcher");
        display.setCursor(3, 27);
        display.print("2. Data Limits");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Configurations");
        display.setCursor(3, 55);
        display.print("4. Reset");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Sys Watcher");
        display.setCursor(3, 27);
        display.print("2. Data Limits");
        display.setCursor(3, 41);
        display.print("3. Configurations");
        display.drawRect(0, 38, 127, 13, 1);
        display.setCursor(3, 55);
        display.print("4. Reset");
        break;
      case 4:
        display.setCursor(3, 13);
        display.print("4. Reset");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("5. Exit");
        break;
      case 5:
        display.setCursor(3, 13);
        display.print("4. Reset");
        display.setCursor(3, 27);
        display.print("5. Exit");
        display.drawRect(0, 24, 127, 13, 1);
        break;
        /*display.setCursor(3, 55);
  display.print("4. Exit");
  display.drawRect(0, 52, 127, 13, 1);*/
    }

    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20, 50);
        } else {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(60);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 5)
          option = 1;
      } else {
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
    display.display();
  }
  display.clearDisplay();
}

void blinkOrange(byte times, byte brightValue, int blinkDuration) {
  if (times == 0) {
    FastLED.setBrightness(brightValue);
    leds[0] = CRGB::Orange;
    FastLED.show();
  } else {
    int i = 0;
    while (i < times) {
      FastLED.setBrightness(brightValue);
      leds[0] = CRGB::Orange;
      FastLED.show();
      delay(blinkDuration);
      leds[0] = CRGB::Black;
      FastLED.show();
      delay(blinkDuration);
      i++;
    }
  }
}

//container function for safe data limit entry
void dataLimit() {
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Set Limits");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic Sensor");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Current Sensor");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic Sensor");
        display.setCursor(3, 27);
        display.print("2. Current Sensor");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic Sensor");
        display.setCursor(3, 27);
        display.print("2. Current Sensor");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);
        break;
        /*case 4:
        display.setCursor(3, 13);
        display.print("4. Wattage Value");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("5. Exit");
        break;
      case 5:
        display.setCursor(3, 13);
        display.print("4. Wattage Value");
        display.setCursor(3, 27);
        display.print("5. Exit");
        display.drawRect(0, 24, 127, 13, 1);
        /*display.setCursor(3, 55);
        display.print("4. Exit");
        display.drawRect(0, 52, 127, 13, 1);
        break;*/
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20, 50);
        } else {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();
      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1)
          ultraSonicValues();
        else if (option == 2)
          ampereValues();
        else if (option == 3)
          return;
      }
    }
    count = 0;
    display.display();
  }
}

//sets values for tankLow, tankFull using live values from sensor
void ultraSonicValues() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Ultra-Sonic Sensor");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);
        /*display.setCursor(3, 55);
        display.print("4. Exit");
        display.drawRect(0, 52, 127, 13, 1);*/
        break;
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20, 50);
        } else {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();
      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1) {
          byte count = 0, option = 1;
          while (true) {
            int newValue = liveTankLevel;  //fetch live ultra-sonic values
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
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();
              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  tankLow = newValue;  //save tankLow value in preference
                  pref.putInt("tankLow", tankLow);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(500);

          while (true) {
            int newValue = liveTankLevel;  //fetch live ultra-sonic values
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
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();
              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  tankFull = newValue;  //save tankFull value in preference
                  pref.putInt("tankFull", tankFull);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2) {
          byte count = 0, option = 1;
          int newValue = tankLow;  //old value
          while (true) {
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
            if (option == 1) {
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
            } else if (option == 2) {
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
            } else if (option == 3) {
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
            } else if (option == 4) {
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2) {
                  newValue++;
                  if (newValue >= MAX_ULTRASONIC_VALUE)
                    newValue = MAX_ULTRASONIC_VALUE;
                } else if (option == 3) {
                  tankLow = newValue;  //save tankLow value in preference
                  pref.putInt("tankLow", tankLow);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();

          display.clearDisplay();
          display.display();
          delay(500);

          count = 0, option = 1;
          newValue = tankFull;  //old value
          while (true) {
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
            if (option == 1) {
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
            } else if (option == 2) {
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
            } else if (option == 3) {
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
            } else if (option == 4) {
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue++;
                else if (option == 3) {
                  tankFull = newValue;  //save tankFull value in preference
                  pref.putInt("tankFull", tankFull);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();
        } else if (option == 3) return;
      }
    }
    count = 0;
    display.display();
  }
  pref.end();
}

//sets values for ampLow, ampMax, wattLow, wattMax using live values from sensor
void ampereValues() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Ampere Sensor");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);
        /*display.setCursor(3, 55);
        display.print("4. Exit");
        display.drawRect(0, 52, 127, 13, 1);*/
        break;
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1) {
          byte count = 0, option = 1;
          while (true) {
            float newValue = liveAmp;  //fetch live ampere sensor values
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
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  ampLow = newValue;  //save ampLow value in preference
                  pref.putFloat("ampLow", ampLow);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(500);

          while (true) {
            float newValue = liveAmp;  //fetch live Ampere values
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
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  ampMax = newValue;  //save ampMax value in preference
                  pref.putFloat("ampMax", ampMax);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2) {
          byte count = 0, option = 1;
          float newValue = ampLow;  //old value
          while (true) {
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
            if (option == 1) {
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
            } else if (option == 2) {
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
            } else if (option == 3) {
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
            } else if (option == 4) {
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue = newValue - 0.1;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue = newValue + 0.1;
                else if (option == 3) {
                  ampLow = newValue;  //save ampLow value in preference
                  pref.putFloat("ampLow", ampLow);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();

          display.clearDisplay();
          display.display();
          delay(500);

          count = 0, option = 1;
          newValue = ampMax;  //old value
          while (true) {
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
            if (option == 1) {
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
            } else if (option == 2) {
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
            } else if (option == 3) {
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
            } else if (option == 4) {
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue = newValue - 0.1;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue = newValue + 0.1;
                else if (option == 3) {
                  ampMax = newValue;  //save ampMax value in preference
                  pref.putFloat("ampMax", ampMax);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();
        } else if (option == 3) return;
      }
    }

    count = 0;
    display.display();
  }
  pref.end();
}
/*
//sets values for voltLow, voltMax using live values from sensor
void voltageValues() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Voltage Sensor");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);        
        break;
    }

    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1) {
          byte count = 0, option = 1;
          while (true) {
            int newValue = liveVoltage;  //fetch live voltage values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Voltage: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(voltLow) + " V");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " V");
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  voltLow = newValue;  //save voltLow value in preference
                  pref.putInt("voltLow", voltLow);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(500);

          while (true) {
            int newValue = liveVoltage;  //fetch live ultra-sonic values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Voltage: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(voltMax) + " V");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " V");
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  voltMax = newValue;  //save voltMax value in preference
                  pref.putInt("voltMax", voltMax);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2) {
          byte count = 0, option = 1;
          int newValue = voltLow;  //old value
          while (true) {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Voltage: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(voltLow) + " V");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1) {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " V ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 2) {
              display.print("- ");
              display.print(String(newValue) + " V ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 3) {
              display.print("- ");
              display.print(String(newValue) + " V ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 4) {
              display.print("- ");
              display.print(String(newValue) + " V ");
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue++;
                else if (option == 3) {
                  voltLow = newValue;  //save voltLow value in preference
                  pref.putInt("voltLow", voltLow);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();

          display.clearDisplay();
          display.display();
          delay(500);

          count = 0, option = 1;
          newValue = voltMax;  //old value
          while (true) {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Voltage: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(voltMax) + " V");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1) {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " V ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 2) {
              display.print("- ");
              display.print(String(newValue) + " V ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 3) {
              display.print("- ");
              display.print(String(newValue) + " V ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 4) {
              display.print("- ");
              display.print(String(newValue) + " V ");
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue++;
                else if (option == 3) {
                  voltMax = newValue;  //save voltMax value in preference
                  pref.putInt("voltMax", voltMax);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();
        } else if (option == 3) return;
      }
    }

    count = 0;
    display.display();
  }
  pref.end();
}

//sets values for wattLow, wattMax using live values from sensor
void wattValues() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Wattage Value");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Use live values");
        display.setCursor(3, 27);
        display.print("2. Use manual values");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);        
        break;
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1) {
          byte count = 0, option = 1;
          while (true) {
            int newValue = liveWatt;  //fetch live voltage values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Wattage: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(wattLow) + " W");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " W");
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  wattLow = newValue;  //save wattLow value in preference
                  pref.putInt("wattLow", wattLow);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          display.clearDisplay();
          display.display();
          delay(500);

          while (true) {
            int newValue = liveWatt;  //fetch live wattage values
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Wattage: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(wattMax) + " W");
            display.setCursor(0, 28);
            display.print("New Value:");
            display.print(String(newValue) + " W");
            //buttons
            if (option == 1) {
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else {
              display.setCursor(22, 50);
              display.print("Save");
              display.fillRect(63, 49, 45, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
              display.setTextColor(SH110X_WHITE);
            }

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (count >= 1 && count <= 2) {
                  blinkOrange(1, 20);
                } else {
                  blinkOrange(0, 150);
                  delay(100);
                }
                delay(50);
              }

              FastLED.setBrightness(20);
              leds[0] = CRGB::Black;
              FastLED.show();

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 2)
                  option = 1;
              } else {
                if (option == 1) {
                  wattMax = newValue;  //save wattMax value in preference
                  pref.putInt("wattMax", wattMax);
                  break;
                } else if (option == 2)
                  break;
              }
            }
            count = 0;
            display.display();
          }
        }

        else if (option == 2) {
          byte count = 0, option = 1;
          int newValue = wattLow;  //old value
          while (true) {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Wattage: LOW");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(wattLow) + " W");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1) {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " W ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 2) {
              display.print("- ");
              display.print(String(newValue) + " W ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 3) {
              display.print("- ");
              display.print(String(newValue) + " W ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 4) {
              display.print("- ");
              display.print(String(newValue) + " W ");
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue++;
                else if (option == 3) {
                  wattLow = newValue;  //save wattLow value in preference
                  pref.putInt("wattLow", wattLow);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }

          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();

          display.clearDisplay();
          display.display();
          delay(500);

          count = 0, option = 1;
          newValue = wattMax;  //old value
          while (true) {
            display.clearDisplay();
            display.setTextColor(SH110X_WHITE);
            display.setTextSize(1);
            display.setFont(NULL);
            display.setCursor(0, 0);
            display.println("Wattage: HIGH");
            display.drawLine(0, 8, 127, 8, 1);
            display.setCursor(0, 15);
            display.print("Old Value:");
            display.print(String(wattMax) + " W");
            display.setCursor(0, 28);
            display.print("New Value: ");
            if (option == 1) {
              display.fillRect(63, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.print("-");
              display.setTextColor(SH110X_WHITE);
              display.print(" " + String(newValue) + " W ");
              display.setCursor(120, 28);
              display.print("+");
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 2) {
              display.print("- ");
              display.print(String(newValue) + " W ");
              display.fillRect(117, 27, 11, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(120, 28);
              display.print("+");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 3) {
              display.print("- ");
              display.print(String(newValue) + " W ");
              display.setCursor(120, 28);
              display.print("+");
              display.fillRect(20, 49, 27, 10, 1);
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              display.setCursor(22, 50);
              display.print("Save");
              display.setTextColor(SH110X_WHITE);
              display.setCursor(65, 50);
              display.print("Discard");
            } else if (option == 4) {
              display.print("- ");
              display.print(String(newValue) + " W ");
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

            if (digitalRead(BUTTON) == 1) {
              while (digitalRead(BUTTON) == 1) {
                count++;
                if (((option == 1) || (option == 2)) && count >= 3)
                  break;
                else if (((option == 3) || (option == 4)) && count >= 3) {
                  delay(100);
                  blinkOrange(0, 150, 0);
                }
                if (count <= 2)
                  blinkOrange(1, 20);
                delay(50);
              }

              if (count >= 1 && count <= 2) {
                option++;
                if (option > 4)
                  option = 1;
              } else {
                if (option == 1) {
                  newValue--;
                  if (newValue < 0)
                    newValue = 0;
                } else if (option == 2)
                  newValue++;
                else if (option == 3) {
                  wattMax = newValue;  //save wattMax value in preference
                  pref.putInt("wattMax", wattMax);
                  break;
                } else if (option == 4)
                  break;
              }
            }
            count = 0;
            display.display();
          }
          FastLED.setBrightness(20);
          leds[0] = CRGB::Black;
          FastLED.show();
        } else if (option == 3) return;
      }
    }

    count = 0;
    display.display();
  }
  pref.end();
}*/

void wifiManagerInfoPrint() {
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

void configurations() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Configurations");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Time Update");
        display.setCursor(3, 41);
        display.print("3. Other Sensors");
        display.setCursor(3, 55);
        display.print("4. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic");
        display.setCursor(3, 27);
        display.print("2. Time Update");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Other Sensors");
        display.setCursor(3, 55);
        display.print("4. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic");
        display.setCursor(3, 27);
        display.print("2. Time Update");
        display.setCursor(3, 41);
        display.print("3. Other Sensors");
        display.drawRect(0, 38, 127, 13, 1);
        display.setCursor(3, 55);
        display.print("4. Exit");
        break;
      case 4:
        display.setCursor(3, 13);
        display.print("1. Ultrasonic");
        display.setCursor(3, 27);
        display.print("2. Time Update");
        display.setCursor(3, 41);
        display.print("3. Other Sensors");
        display.setCursor(3, 55);
        display.print("4. Exit");
        display.drawRect(0, 52, 127, 13, 1);
        break;
        /*display.setCursor(3, 55);
  display.print("4. Exit");
  display.drawRect(0, 52, 127, 13, 1);*/
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 4)
          option = 1;
      } else {
        if (option == 4)
          return;
        else if (option == 1)
          break;
        else if (option == 2)
          configTime();
        else if (option == 3)
          configSensors();  //Bypass Sensors (Current and Float)
      }
    }
    count = 0;
    display.display();
  }
  if (option == 1) {
    while (true) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Ultrasonic");
      display.drawLine(0, 8, 127, 8, 1);
      switch (option) {
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
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1 && count <= 2) {
            blinkOrange(1, 20);
          } else {
            blinkOrange(0, 150);
            delay(100);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1 && count <= 2) {
          option++;
          if (option > 2)
            option = 1;
        } else {
          if (option == 1) {
            useUltrasonic = true;
            pref.putBool("useUltrasonic", true);
            break;
          } else if (option == 2) {
            useUltrasonic = false;
            pref.putBool("useUltrasonic", false);
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

void configSensors() {
  pref.begin("database", false);
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Configurations");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Current Sensor");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Float Sensor");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Current Sensor");
        display.setCursor(3, 27);
        display.print("2. Float Sensor");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Current Sensor");
        display.setCursor(3, 27);
        display.print("2. Float Sensor");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);
        break;
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1)
          break;
        else if (option == 2)
          break;
        else if (option == 3)
          return;
      }
    }
    count = 0;
    display.display();
  }
  if (option == 1) {
    while (true) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Current Sensor");
      display.drawLine(0, 8, 127, 8, 1);
      switch (option) {
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
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1 && count <= 2) {
            blinkOrange(1, 20);
          } else {
            blinkOrange(0, 150);
            delay(100);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1 && count <= 2) {
          option++;
          if (option > 2)
            option = 1;
        } else {
          if (option == 1) {
            useSensors = true;
            pref.putBool("useSensors", true);
            break;
          } else if (option == 2) {
            useSensors = false;
            pref.putBool("useSensors", false);
            break;
          }
        }
      }
      count = 0;
      display.display();
    }
  } else if (option == 2) {
    while (true) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setFont(NULL);
      display.setCursor(0, 0);
      display.println("Float Sensor");
      display.drawLine(0, 8, 127, 8, 1);
      switch (option) {
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
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1 && count <= 2) {
            blinkOrange(1, 20);
          } else {
            blinkOrange(0, 150);
            delay(100);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1 && count <= 2) {
          option++;
          if (option > 2)
            option = 1;
        } else {
          if (option == 1) {
            useFloat = true;
            pref.putBool("useFloat", true);
            break;
          } else if (option == 2) {
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

//Manual time set not implemented yet
void configTime() {
  byte count = 0, option = 1;
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.println("Time Update");
    display.drawLine(0, 8, 127, 8, 1);
    switch (option) {
      case 1:
        display.setCursor(3, 13);
        display.print("1. Auto (Internet)");
        display.drawRect(0, 10, 127, 13, 1);
        display.setCursor(3, 27);
        display.print("2. Manual");
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 2:
        display.setCursor(3, 13);
        display.print("1. Auto (Internet)");
        display.setCursor(3, 27);
        display.print("2. Manual");
        display.drawRect(0, 24, 127, 13, 1);
        display.setCursor(3, 41);
        display.print("3. Exit");
        break;
      case 3:
        display.setCursor(3, 13);
        display.print("1. Auto (Internet)");
        display.setCursor(3, 27);
        display.print("2. Manual");
        display.setCursor(3, 41);
        display.print("3. Exit");
        display.drawRect(0, 38, 127, 13, 1);
        break;
        /*display.setCursor(3, 55);
  display.print("4. Exit");
  display.drawRect(0, 52, 127, 13, 1);*/
    }
    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 3)
          option = 1;
      } else {
        if (option == 1)
          autoTimeUpdate();
        else if (option == 2)
          break;  //implement this
        else if (option == 3)
          return;
      }
    }
    count = 0;
    display.display();
  }
  display.clearDisplay();
}

//Check this for disconnection error
void autoTimeUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    if (timeClient.update()) {
      time_t rawtime = timeClient.getEpochTime();
      struct tm* ti;
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
    while (1) {
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1) {
            blinkOrange(1, 20);
          }
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1)
          break;
      }
    }
  } else {
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
    while (1) {
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1)
            blinkOrange(1, 20);
          delay(50);
        }

        FastLED.setBrightness(20);
        leds[0] = CRGB::Black;
        FastLED.show();

        if (count >= 1)
          break;
      }
    }
  }
  return;
}

void totalReset() {
  byte count = 0, option = 1;
  FastLED.setBrightness(150);
  leds[0] = CRGB::Red;
  FastLED.show();
  while (true) {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(0, 10);
    display.println("Reset Everything?");
    //buttons
    if (option == 1) {
      display.setCursor(20, 40);
      display.print("YES");
      display.setCursor(90, 40);
      display.fillRect(88, 39, 15, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("NO");
      display.setTextColor(SH110X_WHITE);
    } else {
      display.setCursor(20, 40);
      display.fillRect(18, 39, 21, 10, 1);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("YES");
      display.setTextColor(SH110X_WHITE);
      display.setCursor(90, 40);
      display.print("NO");
    }

    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20);
        } else {
          blinkOrange(0, 150);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();

      if (count >= 1 && count <= 2) {
        option++;
        if (option > 2)
          option = 1;
      } else {
        if (option == 1)  //NO
          break;
        else if (option == 2) {  //YES
          pref.begin("database", false);

          pref.putString("ssid", "");
          pref.putString("password", "");
          pref.putInt("tankLow", 0);
          pref.putInt("tankFull", 0);
          pref.putFloat("ampLow", 0);
          pref.putFloat("ampMax", 0);
          /*pref.putInt("wattLow", 0);
          pref.putInt("wattMax", 0);
          pref.putInt("voltLow", 0);
          pref.putInt("voltMax", 0);*/
          pref.putBool("useUltrasonic", false);
          pref.putBool("useSensors", false);
          pref.putBool("useFloat", false);

          pref.end();
          resetFlag = true;
        }
      }
    }
    count = 0;
    display.display();
  }
}

/*prints various error messages on screen
\n tank full: err = 2
\n high ampere: err = 3
\n high watt: err = 4
\n dry run: err = 5
\n low voltage: err = 6
\n high voltage: err = 7
*/
void errorMsg(byte code) {
  FastLED.setBrightness(150);
  leds[0] = CRGB::Red;
  FastLED.show();

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 10);
  display.print("Msg:");
  if (code == 2)
    display.print(" TANK FULL");
  else if (code == 3)
    display.print(" HIGH AMPERE");
  else if (code == 4)
    display.print(" HIGH WATT");
  else if (code == 5)
    display.print(" DRY RUN");
  else if (code == 6)
    display.print(" LOW VOLTAGE");
  else if (code == 7)
    display.print(" HIGH VOLTAGE");
  else if (code == 8)
    display.print(" LOW AMPERE");
  display.display();
  digitalWrite(BUZZER_PIN, HIGH);

  while (1) {
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(50, 41);
    display.fillRect(47, 40, 29, 10, 1);
    display.print("OKAY");
    display.setTextColor(SH110X_WHITE);
    display.display();
    delay(50);

    byte count = 0;
    while (1) {
      if (digitalRead(BUTTON) == 1) {
        while (digitalRead(BUTTON) == 1) {
          count++;
          if (count >= 1) {
            blinkOrange(1, 20);
          }
          delay(50);
        }

        if (count >= 1) {
          digitalWrite(BUZZER_PIN, LOW);
          return;
        }
      }
    }
  }
}

/*
Prints various system related data
!! Add Current Sensor and Ultrasonic Sensor
*/
void sysWatcher() {
  byte data = 0, option = 0, count = 0;
  while (1) {
    if (data == 0) {
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
    } else if (data == 1) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Float: " + String(analogRead(FLOAT_SENSOR)));
    }

    if (option == 0) {
      display.setCursor(32, 56);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(28, 55, 29, 10, 1);
      display.print("BACK");
      display.setTextColor(SH110X_WHITE);
      display.setCursor(75, 56);
      display.print("NEXT");
      display.drawCircle(70, 50, 2, 1);
      display.fillCircle(59, 50, 2, 1);
      display.setTextColor(SH110X_WHITE);
      display.display();
    } else if (option == 1) {
      display.setCursor(32, 56);
      display.setTextColor(SH110X_WHITE);
      display.fillRect(28, 55, 29, 10, 1);
      display.print("BACK");
      display.setCursor(75, 56);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.fillRect(71, 55, 29, 10, 1);
      display.print("NEXT");
      display.fillCircle(70, 50, 2, 1);
      display.drawCircle(59, 50, 2, 1);
      display.setTextColor(SH110X_WHITE);
      display.display();
    }

    if (digitalRead(BUTTON) == 1) {
      while (digitalRead(BUTTON) == 1) {
        count++;
        if (count >= 1 && count <= 2) {
          blinkOrange(1, 20, 50);
        } else {
          blinkOrange(0, 150, 0);
          delay(100);
        }
        delay(50);
      }

      FastLED.setBrightness(20);
      leds[0] = CRGB::Black;
      FastLED.show();
      if (count >= 1 && count <= 2) {
        option++;
        if (option > 1)
          option = 0;
      } else {
        if (option == 0)
          return;
        else if (option == 1) {
          data++;
          if (data > 1)
            data = 0;
        }
      }
    }
    count = 0;
  }
}

void WiFiEvent(WiFiEvent_t event) {
  if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
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
