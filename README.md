# Advanced Water Pump Controller
###A pump controller with dry running protection, time-left to fill, diagnostics and other smart features.

## Hardware:
1. DOIT ESP32 DEVKIT V1 (30 pin)
2. 128x64 OLED Display
3. ZMPT101B Voltage Sensor
4. DS1307 RTC
5. SCT013 30A/1V Current sensor
6. AJ-SR04 Ultrasonic Sensor(Water Proof)
7. WS2812B LED (Single LED Module)
8. Any Push Button (I used metal one, feels sturdy)
9. Any desired float sensor
10. Wires, connectors, etc (according to your need)

## Upcoming Features
Check [Issues](https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues)

## Pictures, Schematics and other stuff (COMING SOON, check ***issues***)
<img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/SCH_Schematic1.png" alt="Schematics" width="500" height="600">

## Some of the Features
1. Dual core utilisation of ESP32, one core working with UI and another monitoring sensors.
2. Full Induction Pump Protection System, tested on 240V 1HP Water Pump (*)
3. Range of parameters for customisation, uses Preferences Library for remembering your choices.
4. Uses Float Sensor for basic water level sensing and turning off of PUMP with Advanced Waterproof Distance sensor to show real time water level (*)
5. Uses one button system, SINGLE PRESS (SHORT) for navigation and SINGLE PRESS (LONG) for selection, with flashing LEDs for guidance.
6. 1.3 inches 128x64 OLED Display, RGB LEDs and Buzzers makeup for total user interaction and experience.
7. RTC DS1307 can be used for time dependent operations and further automation in entirety (User needs to implement it according to their need, I will work upon it later)
8. OTA Update supported (local network as of now)
9. WiFi Manager for seamlessly setting up the device with your smartphone with on-screen guidance. RTC Clock updates automatically with Wifi (Go to the settings).
10. More Coming Soon.

(*) ***various parameters needed to be set based on user needs and scenarious*** </br>
** ***Sensors used in this project are hobby level, please don't expect industry standards. Sensors may fail, devices may fail. Be cautious and use it at your own risk*** </br>
*** ***Deals with high voltage current, be ultra cautious***

