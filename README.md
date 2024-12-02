# Advanced Water Pump Controller
## A pump/induction motor controller with dry running protection, timer, error detection, time-left to fill, diagnostics and other smart features.

## Hardware:
1. Tested on: DOIT ESP32 DEVKIT V1 (30 pin)
2. 128x64 OLED Display <br>
~3. ZMPT101B Voltage Sensor~
4. DS1307 RTC
5. SCT013 30A/1V Current sensor (Check for [Wiring](https://simplyexplained.com/blog/Home-Energy-Monitor-ESP32-CT-Sensor-Emonlib/))
6. AJ-SR04 Ultrasonic Sensor(Water Proof) (Check [Mode 2](https://www.makerguides.com/interfacing-esp32-and-jsn-sr04t-waterproof-ultrasonic-sensor/#Mode_2_of_JSN-SR04T_Sensor) for connection)
7. WS2812B LED (Single LED Module)
8. Any Push Button (I used metal one, feels sturdy)
9. Any desired float sensor
10. Wires, connectors, etc (according to your need)
11. Fotek 100Amps SSR

## Upcoming Features
Check [Issues](https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues)

## Pictures, Schematics and other stuff
<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/SCH_Schematic1.png" alt="Schematics" width="400" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x1.jpg" alt="pictures-of-controller" width="400" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x2.jpg" alt="pictures-of-controller" width="400" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x3.jpg" alt="pictures-of-controller" width="400" height="500">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x4.jpg" alt="pictures-of-controller" width="400" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x5.jpg" alt="pictures-of-controller" width="500" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x6.jpg" alt="pictures-of-controller" width="400" height="300">

<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x7.jpg" alt="pictures-of-controller" width="400" height="500">

## Some of the Features
1. Dual core utilisation of ESP32, one core working with UI and another monitoring sensors.
2. Full Induction Pump Protection System, tested on 240V 1HP Water Pump (*)
3. SENSORS CAN BE TURNED OFF FROM THE MENU, GIVING YOU THE OPTION TO MAKE THE PROJECT LIGHTER!!
4. Range of parameters for customisation, it uses Preferences Library for remembering your choices.
5. Uses Float Sensor for basic water level sensing and turning off of PUMP with Advanced Waterproof Distance sensor to show real time water level (*)
6. It uses a one-button system, SINGLE PRESS (SHORT) for navigation and SINGLE PRESS (LONG) for selection, with flashing LEDs for guidance.
7. 1.3 inches 128x64 OLED Display, RGB LEDs and Buzzers make up for total user interaction and experience.
8. RTC DS1307 can be used for time-dependent operations and further automation in its entirety (User needs to implement it according to their need, I will work on it later)
9. OTA Update supported (local network as of now)
10. WiFi Manager for seamlessly setting up the device with your smartphone with on-screen guidance. RTC Clock updates automatically with Wifi (Go to the settings).
11. More Coming Soon.

(*) ***Various parameters need to be set based on user needs and scenarios. For the calibration of sensors, various limits need to be set through the program menu. Feel free to let me know if you need help.*** </br>
** ***Sensors used in this project are hobby level, please don't expect industry standards. Sensors may fail, and devices may fail. Be cautious and use it at your own risk*** </br>
*** ***Deals with high voltage current, be ultra cautious***

## Temporary Fix
The program works great just using Current Sensor, Float Sensor and Distance Sensor (Assuming you have some kind of Voltage protection system in your house). Even without Voltage monitoring, you can kinda predict it using Current i.e. When Voltage Rises Current drops, and When Voltage drops Current rises.
REMEMBER TO CALIBRATE THE CURRENT SENSOR!! The easiest way is to verify it with an Electric Meter/Energy Meter installed by your electricity company and adjust the calibration value, for me, it is 27.
In my case, I get 220-240 volts (from the energy meter), and the ampere is around 2.6 A to 2.9 A for 1 HP Induction Motor. And I have a digital Electricity protection system in my house.

## The Logging Functionality
This program can now log time-taken to fill the tank and other parameters in google sheet. I have used Pythonanywheere for receiving those data from ESP32 in JSON/POST format. The program in Pythonanywhere adds it to Google Sheets. Follow up to step 2 of [this](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/) for getting credentials from Google Cloud. Then you can check the "resources" folder for the Python code written for FLASK, which writes the data in Google sheet. Please let me know if you have any doubts.
There is a different version of this program, which directly stores the data in Google Sheets without using any middleman, it is stored in the "resources" folder as well. PLEASE CHECK THE HEADER COMMENT of that program for board selection and partition scheme information. I am not using this version because it takes a lot of memory but you can always change the partition scheme and use it.

### Known Issues 1.2.0
* Same as 1.1.1-beta
* Ultrasonic works as of now (when it is connected), because the water is falling from height and waves are forming, hence accuracy is affected.
### Known Issues 1.1.1-beta
* When ultrasonic is not connected, but turned on in settings, the ESP32 restarts in loop. The primary issue is that the Serial Monitor for ultrasonic returns garbage values even when nothing is connected and ESP32 tries to make sense of it and fails. The secondary issue is that it is running on CORE 0 nd hence there is some kind of issue that is not faced by CORE 1 a.k.a. loop(). I'll check it in detail later.
* Removed voltage sensor completely for now.
  
### Known Issues 1.1.0-beta
1. Same as 1.0.2-alpha

### Known Issues 1.0.3-alpha
1. Same as 1.0.2-alpha

### Known Issues 1.0.2-alpha
1. Voltage Monitoring is not working (Giving higher values than individual test results, will look into it later).
2. Wattage is dependent on voltage, so it is also not working.
