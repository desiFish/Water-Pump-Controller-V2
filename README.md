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
</div>

<div align="center">
  <table>
    <tr>
      <td>
        <h3>⚠️ Important Disclaimer</h3>
        <p align="left">
          <b>Project Status:</b> Active Development & Testing<br>
          This is a new project under continuous development. Features, documentation, and test data will be updated regularly.
        </p>
        <p align="left">
          <b>Safety Notice:</b><br>
          • This project uses hobby-grade components and is NOT designed for 24/7 commercial operation<br>
          • While safety features are implemented, they should not be considered fail-safe<br>
          • Use appropriate commercial-grade equipment for critical applications<br>
          • No liability is accepted for any damages resulting from using this project
        </p>
      </td>
    </tr>
  </table>
</div>

## 🎯 Key Features & Capabilities

<details open>
<summary><b>💪 Core Features</b></summary>

- `⚡ Dual Core ESP32` - One core for UI, one for sensor monitoring
- `🔄 OTA Updates` - Over-the-air firmware updates via local network
- `📱 WiFi Manager` - Easy device setup using smartphone
- `💾 Preferences` - Persistent settings storage
</details>

<details open>
<summary><b>🛡️ Protection Systems</b></summary>

- `🚱 Dry Run Protection` - Prevents pump damage from running dry
- `⚠️ Overload Detection` - Monitors current draw and shuts off if exceeded
- `🔌 Short Circuit` - Advanced electrical protection features
- `📊 Load Monitoring` - Real-time current and power monitoring
</details>

<details open>
<summary><b>🎮 Smart Controls</b></summary>

- `🔘 One-Button System` - Short press to navigate, long press to select
- `📟 OLED Display` - 128x64 clear visual interface
- `🚥 RGB Indicators` - Status and warning indicators
- `⏲️ Timer Controls` - Scheduled operations
</details>

<details open>
<summary><b>📊 Monitoring & Logging</b></summary>

- `💧 Water Level` - Real-time tank level monitoring
- `📈 Google Sheets` - Data logging and analysis
- `🔍 Diagnostics` - Comprehensive system monitoring
- `⚡ Power Stats` - Current and power consumption tracking
</details>

<br>

> **Note:** Settings and parameters can be customized through an intuitive menu system.

<table>
<tr>
<td align="center">
<img src="https://img.shields.io/badge/ESP32-Dual_Core-blue?style=for-the-badge&logo=arduino" alt="ESP32 Dual Core"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/Protection-Advanced-red?style=for-the-badge&logo=shield" alt="Advanced Protection"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/Interface-OLED-green?style=for-the-badge&logo=display" alt="OLED Interface"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/Logging-Enabled-purple?style=for-the-badge&logo=googlesheets" alt="Logging Enabled"/>
</td>
</tr>
</table>

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

### System Architecture
- **Dual Core Processing**
  - Core 1: Handles user interface and display updates
  - Core 2: Dedicated to real-time sensor monitoring and safety checks
- **Memory Management**
  - Configurable sensor enables/disables to optimize memory usage
  - Persistent storage using Preferences Library for all settings

### Protection Systems
- **Pump Protection**
  - Dry run detection and automatic shutdown
  - Current-based overload protection
  - Tested with 240V 1HP water pump
  - Real-time current and power monitoring
- **Safety Features**
  - Automatic shutdown on abnormal conditions
  - Configurable threshold limits for all parameters
  - Emergency stop functionality

### Smart Controls
- **One-Button Interface**
  - Short press: Navigate through menus
  - Long press: Select/Confirm options
  - LED feedback for button actions
- **Display System**
  - 1.3" 128x64 OLED display for clear visibility
  - RGB LED status indicators
  - Buzzer alerts for critical notifications

### Water Level Management
- **Dual Sensor System**
  - Primary: Float sensor for reliable level detection
  - Secondary: Waterproof ultrasonic sensor for real-time level monitoring
  - Automatic pump control based on tank levels

### Connectivity
- **WiFi Capabilities**
  - Built-in WiFi Manager for easy setup
  - Smartphone-based configuration
  - OTA (Over-The-Air) firmware updates
  - Automatic RTC synchronization via WiFi

### Time Management
- **RTC Integration**
  - DS1307 Real-Time Clock
  - Scheduled operations support
  - Time-based automation features
  - Power failure time tracking

### Data Logging
- **Remote Monitoring**
  - Google Sheets integration for data logging
  - Fill time analytics
  - Performance tracking
  - Remote status monitoring

⚠️ **Configuration Notes:**
- All protection parameters are user-configurable through the menu system
- Initial calibration required for optimal performance
- Refer to calibration section for sensor setup guidelines

> 💡 **Future Updates:** More features are being developed. Check the Issues section for upcoming additions.
</div>

<div style="background-color: #F5EEF8; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 📊 Data Logging System

### Overview
This project offers two methods for logging pump data to Google Sheets:
1. Using PythonAnywhere as middleware (Recommended)
2. Direct Google Sheets integration (Advanced)

### Method 1: Using PythonAnywhere (Recommended)
#### Setup Process
1. **Google Cloud Setup**
   - Follow [this guide](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/) up to step 2
   - Obtain necessary Google Cloud credentials
   - Set up your Google Sheet for data reception

2. **PythonAnywhere Configuration**
   - Deploy provided Flask code to PythonAnywhere
   - Code available in `/resources` folder
   - Handles JSON data from ESP32
   - Manages Google Sheets communication

3. **Data Collection**
   - Records fill time duration
   - Logs sensor parameters
   - Tracks system performance
   - Stores historical data

### Method 2: Direct Integration (Advanced)
#### Important Notes
- Located in `/resources` folder
- Requires different partition scheme
- Higher memory usage
- Check header comments for:
  - Board selection details
  - Partition scheme requirements
  - Setup instructions

### Logged Parameters
- Tank fill duration
- Water level readings
- Current consumption
- System status
- Error conditions
- Performance metrics

> 💡 **Tip:** Choose Method 1 for better memory management and system stability. Use Method 2 only if you need direct integration and have configured the appropriate partition scheme.

### Data Analysis
- Track system efficiency
- Monitor fill patterns
- Identify potential issues
- Analyze performance trends
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

<div style="background-color: #F5F5F5; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⚙️ Calibration & Setup

### Core Sensors
The controller relies on three primary sensors for optimal operation:
- Current Sensor (SCT013)
- Float Sensor
- Distance Sensor (Ultrasonic)

### Current Sensor Calibration
1. **Initial Setup**
   - Default calibration value: 27 (adjustable through program)
   - Reference: Use your home's electricity meter readings
   - Target: Match controller readings with meter readings

2. **Expected Readings** (1HP Pump)
   - Voltage Range: 220-240V AC
   - Current Draw: 2.6A - 3.3A
   - Note: Readings may vary based on your pump specifications

3. **Calibration Process**
   ```
   1. Run pump under normal conditions
   2. Compare controller readings with electricity meter
   3. Adjust calibration value until readings match
   4. Test at different times to ensure consistency
   ```

### Important Notes
- **Voltage Protection:** External voltage protection is recommended
- **Current-Voltage Relationship:**
  - Current increases when voltage drops
  - Current decreases when voltage rises
  - This relationship helps detect power quality issues

### Safety Recommendations
1. Always have external circuit protection
2. Monitor initial readings carefully
3. Document your calibration values
4. Perform periodic calibration checks

> 💡 **Tip:** The system can operate reliably without voltage monitoring, as current measurements provide indirect voltage information through their inverse relationship.
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
