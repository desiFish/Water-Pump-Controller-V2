<div align="center">
  <h1 style="color: #2E86C1;">🌊 Advanced Water Pump Controller</h1>
  <p style="font-size: 1.2em; color: #5499C7;">A smart pump/induction motor controller with advanced features and protection systems</p>

  <!-- Badges -->
  <p>
    <img src="https://img.shields.io/github/v/release/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?include_prereleases&style=flat-square" alt="Release">
    <img src="https://img.shields.io/github/license/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="License">
    <img src="https://img.shields.io/github/languages/top/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="Top Language">
    <img src="https://img.shields.io/github/issues/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="Issues">
    <img src="https://img.shields.io/github/last-commit/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="Last Commit">
    <img src="https://img.shields.io/github/repo-size/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="Repo Size">
    <img src="https://img.shields.io/github/stars/KamadoTanjiro-beep/Advanced-Water-Pump-Controller?style=flat-square" alt="Stars">
    <img src="https://img.shields.io/badge/ESP32-Ready-blue?style=flat-square" alt="ESP32 Ready">
    <img src="https://img.shields.io/badge/Arduino-Compatible-green?style=flat-square" alt="Arduino Compatible">
  </p>
  
  <div style="background-color: #EBF5FB; padding: 30px; border-radius: 12px; margin: 20px 0; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
    <h3 style="color: #2874A6; margin-bottom: 25px; text-align: center; font-size: 24px;">🛠️ Key Features</h3>
    
    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px;">
      <!-- Protection Features -->
      <div style="background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #3498DB;">
        <h4 style="color: #2E86C1; margin: 0 0 15px 0; display: flex; align-items: center;">
          <span style="font-size: 24px; margin-right: 10px;">🛡️</span> Protection Features
        </h4>
        <ul style="list-style: none; padding: 0; margin: 0;">
          <li style="margin-bottom: 8px;">✅ Dry Running Protection</li>
          <li style="margin-bottom: 8px;">✅ Overload Protection</li>
          <li style="margin-bottom: 8px;">✅ Short Circuit Safety</li>
        </ul>
      </div>

      <!-- Smart Controls -->
      <div style="background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #E74C3C;">
        <h4 style="color: #2E86C1; margin: 0 0 15px 0; display: flex; align-items: center;">
          <span style="font-size: 24px; margin-right: 10px;">⚡</span> Smart Controls
        </h4>
        <ul style="list-style: none; padding: 0; margin: 0;">
          <li style="margin-bottom: 8px;">⏲️ Timer Controls</li>
          <li style="margin-bottom: 8px;">🔄 Auto Restart</li>
          <li style="margin-bottom: 8px;">📱 WiFi Management</li>
        </ul>
      </div>

      <!-- Monitoring -->
      <div style="background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #2ECC71;">
        <h4 style="color: #2E86C1; margin: 0 0 15px 0; display: flex; align-items: center;">
          <span style="font-size: 24px; margin-right: 10px;">📊</span> Monitoring
        </h4>
        <ul style="list-style: none; padding: 0; margin: 0;">
          <li style="margin-bottom: 8px;">🔍 Real-time Diagnostics</li>
          <li style="margin-bottom: 8px;">💾 Data Logging</li>
          <li style="margin-bottom: 8px;">📈 Performance Tracking</li>
        </ul>
      </div>

      <!-- Hardware -->
      <div style="background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #9B59B6;">
        <h4 style="color: #2E86C1; margin: 0 0 15px 0; display: flex; align-items: center;">
          <span style="font-size: 24px; margin-right: 10px;">🔧</span> Hardware
        </h4>
        <ul style="list-style: none; padding: 0; margin: 0;">
          <li style="margin-bottom: 8px;">💻 Dual Core ESP32</li>
          <li style="margin-bottom: 8px;">🎛️ OLED Display</li>
          <li style="margin-bottom: 8px;">🔌 Multiple Sensors</li>
        </ul>
      </div>
    </div>
  </div>
</div>

<div style="background: linear-gradient(to right, #E8F8F5, #D1F2EB); padding: 20px; border-radius: 10px;">

<div style="background-color: #F4ECF7; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 🔧 Hardware
| Item | Description | Notes |
|------|-------------|-------|
| 1.   | Tested on: DOIT ESP32 DEVKIT V1 (30 pin) | |
| 2.   | 128x64 OLED Display | |
| 3.   | ~~ZMPT101B Voltage Sensor~~ | Removed |
| 4.   | DS1307 RTC | |
| 5.   | SCT013 30A/1V Current sensor | Check for [Wiring](https://simplyexplained.com/blog/Home-Energy-Monitor-ESP32-CT-Sensor-Emonlib/) |
| 6.   | AJ-SR04 Ultrasonic Sensor(Water Proof) | Check [Mode 2](https://www.makerguides.com/interfacing-esp32-and-jsn-sr04t-waterproof-ultrasonic-sensor/#Mode_2_of_JSN-SR04T_Sensor) for connection |
| 7.   | WS2812B LED (Single LED Module) | |
| 8.   | Any Push Button | I used metal one, feels sturdy |
| 9.   | Any desired float sensor | |
| 10.  | Wires, connectors, etc | According to your need |
| 11.  | Fotek 100Amps SSR | |
</div>

<div style="background-color: #FEF9E7; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 🚀 Upcoming Features
Check [Issues](https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues)
</div>

<div style="background-color: #FEF9E7; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 📸 Pictures & Schematics
<table style="width:100%; border-collapse: collapse; margin: 20px 0;">
  <tr>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/SCH_Schematic1.png" alt="Schematics" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Circuit Schematic</p>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x1.jpg" alt="Controller View 1" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Front View</p>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x2.jpg" alt="Controller View 2" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Front View 2</p>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x3.jpg" alt="Controller View 3" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Front View 3</p>
    </td>
  </tr>
  <tr>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x4.jpg" alt="Controller View 4" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Full View</p>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x6.jpg" alt="Controller View 6" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Ultra Sonic</p>
    </td>
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x5.jpg" alt="Controller View 5" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Ultra Sonic USB</p>
    </td>    
    <td align="center" style="padding: 10px;">
      <img src="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/blob/main/resource/x7.jpg" alt="Controller View 7" style="max-width: 300px; height: auto;">
      <p style="margin-top: 5px; font-size: 0.9em;">Complete Setup</p>
    </td>
  </tr>
</table>
</div>

<div style="background-color: #EBF5FB; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⭐ Features
<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 10px;">
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
</div>

<div style="background-color: #FDEDEC; padding: 15px; border-radius: 8px; margin-top: 15px;">
⚠️ <strong>Important Notes:</strong>
(*) ***Various parameters need to be set based on user needs and scenarios. For the calibration of sensors, various limits need to be set through the program menu. Feel free to let me know if you need help.*** </br>
** ***Sensors used in this project are hobby level, please don't expect industry standards. Sensors may fail, and devices may fail. Be cautious and use it at your own risk*** </br>
*** ***Deals with high voltage current, be ultra cautious***
</div>
</div>

<div style="background-color: #E8F8F5; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⚙️ CALIBRATION
The program works great just using Current Sensor, Float Sensor and Distance Sensor (Assuming you have some kind of Voltage protection system in your house). Even without Voltage monitoring, you can kinda predict it using Current i.e. When Voltage Rises Current drops, and When Voltage drops Current rises.
REMEMBER TO CALIBRATE THE CURRENT SENSOR!! The easiest way is to verify it with an Electric Meter/Energy Meter installed by your electricity company and adjust the calibration value, for me, it is 27.
In my case, I get 220-240 volts (from the energy meter), and the ampere is around 2.6 A to 2.9 A for 1 HP Induction Motor. And I have a digital Electricity protection system in my house.
</div>

<div style="background-color: #F5EEF8; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 📊 Logging Functionality
This program can now log time-taken to fill the tank and other parameters in google sheet. I have used Pythonanywheere for receiving those data from ESP32 in JSON/POST format. The program in Pythonanywhere adds it to Google Sheets. Follow up to step 2 of [this](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/) for getting credentials from Google Cloud. Then you can check the "resources" folder for the Python code written for FLASK, which writes the data in Google sheet. Please let me know if you have any doubts.
There is a different version of this program, which directly stores the data in Google Sheets without using any middleman, it is stored in the "resources" folder as well. PLEASE CHECK THE HEADER COMMENT of that program for board selection and partition scheme information. I am not using this version because it takes a lot of memory but you can always change the partition scheme and use it.
</div>

<div style="background-color: #FADBD8; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⚠️ ISSUES
<details>
<summary style="cursor: pointer; font-weight: bold;">Known Issues 1.2.0</summary>
1. Same as 1.1.1-beta
2. Ultrasonic works as of now (when it is connected), because the water is falling from height and waves are forming, hence accuracy is affected.
</details>

<details>
<summary style="cursor: pointer; font-weight: bold;">Known Issues 1.1.1-beta</summary>
1. When ultrasonic is not connected, but turned on in settings, the ESP32 restarts in loop. The primary issue is that the Serial Monitor for ultrasonic returns garbage values even when nothing is connected and ESP32 tries to make sense of it and fails. The secondary issue is that it is running on CORE 0 and hence there is some kind of issue that is not faced by CORE 1 a.k.a. loop(). I'll check it in detail later.
2. Removed voltage sensor completely for now.
</details>

<details>
<summary style="cursor: pointer; font-weight: bold;">Known Issues 1.1.0-beta</summary>
1. Same as 1.0.2-alpha
</details>

<details>
<summary style="cursor: pointer; font-weight: bold;">Known Issues 1.0.3-alpha</summary>
1. Same as 1.0.2-alpha
</details>

<details>
<summary style="cursor: pointer; font-weight: bold;">Known Issues 1.0.2-alpha</summary>
1. Voltage Monitoring is not working (Giving higher values than individual test results, will look into it later).
2. Wattage is dependent on voltage, so it is also not working.
</details>
</div>

<div align="center" style="margin-top: 40px; padding: 20px; background-color: #F8F9F9; border-radius: 10px;">
  <p>Made with ❤️ for the IoT Community</p>
  <p>
    <a href="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues">Report Bug</a> ·
    <a href="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues">Request Feature</a>
  </p>
</div>

<div style="background-color: #F5F5F5; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 📝 License

This project is licensed under the GNU General Public License v3.0 - see below for details.

```
                    GNU GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

                            Preamble

  The GNU General Public License is a free, copyleft license for
  software and other kinds of works.

  When we speak of free software, we are referring to freedom, not
  price. Our General Public Licenses are designed to make sure that
  you have the freedom to distribute copies of free software (and
  charge for them if you wish), that you receive source code or can
  get it if you want it, that you can change the software or use
  pieces of it in new free programs, and that you know you can do
  these things.

  To protect your rights, we need to prevent others from denying
  you these rights or asking you to surrender the rights. Therefore,
  you have certain responsibilities if you distribute copies of the
  software, or if you modify it: responsibilities to respect the
  freedom of others.
```

For the complete license text, see the [LICENSE](LICENSE) file in this repository or visit [GNU GPL v3](https://www.gnu.org/licenses/gpl-3.0.en.html).

### Key Points of GPL v3:
- You can freely use, modify, and distribute this software
- If you distribute modified versions, you must:
  - Make the source code available
  - License it under GPL v3
  - State your modifications
- No warranty is provided
</div>
