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
        <h3>ℹ️ Important Information</h3>
        <p align="left">
          • This project is an advanced version of the <a href="https://github.com/desiFish/ESP8266-Water-Pump-Controller">ESP8266 Water Pump Controller</a>, offering enhanced features and capabilities.<br>
          • The tank level display utilizes a sophisticated stability algorithm, which may result in a slight delay in updates to minimize fluctuations and improve accuracy.<br>
          • This project is under active development. Features, documentation, and performance are continuously being improved and updated.
        </p>
      </td>
    </tr>
  </table>
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

<details>
<summary><b>💪 Core Features</b></summary>

- `⚡ Dual Core ESP32` - One core for UI, one for sensor monitoring
- `🔄 OTA Updates` - Over-the-air firmware updates via local network
- `📱 WiFi Manager` - Easy device setup using smartphone
- `💾 Preferences` - Persistent settings storage
</details>

<details>
<summary><b>🛡️ Protection Systems</b></summary>

- `🚱 Dry Run Protection` - Prevents pump damage from running dry
- `⚠️ Overload Detection` - Monitors current draw and shuts off if exceeded
- `🔌 Short Circuit` - Advanced electrical protection features
- `📊 Load Monitoring` - Real-time current and power monitoring
</details>

<details>
<summary><b>🎮 Smart Controls</b></summary>

- `🔘 One-Button System` - Short press to navigate, long press to select
- `📟 OLED Display` - 128x64 clear visual interface
- `🚥 RGB Indicators` - Status and warning indicators
- `⏲️ Timer Controls` - Scheduled operations
</details>

<details>
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

### 🎛️ System Architecture
- **🧮 Dual Core Processing**
  - 🖥️ Core 1: User Interface & Display
    - 🎨 UI Rendering
    - 📱 Menu System
    - 🔄 Display Updates
  - 🤖 Core 2: Sensor & Safety
    - 📊 Real-time Monitoring
    - ⚡ Power Management
    - 🛡️ Protection Systems

- **💾 Memory System**
  - 🎚️ Dynamic Sensor Configuration
  - 📦 Persistent Settings Storage
  - 🔧 Resource Optimization

### 🛡️ Protection Systems
- **💧 Pump Safety**
  - 🚱 Dry Run Prevention
    - 🔍 Continuous Monitoring
    - 🛑 Auto Shutdown
  - ⚡ Overload Protection
    - 📈 Current Monitoring
    - 🚨 Threshold Alerts
  - 🔌 Short Circuit Guard
    - ⚠️ Fault Detection
    - 🚫 Emergency Cutoff

### 🎮 Smart Controls
- **🔘 One-Button Interface**
  - 👆 Short Press Navigation
  - 👇 Long Press Selection
  - 🌈 RGB Status Feedback
- **📟 Display System**
  - 📱 1.3" OLED Screen
  - 📊 Real-time Stats
  - 🚦 Status Indicators
  - 🔊 Audio Alerts

### 💧 Water Management
- **📏 Level Monitoring**
  - 🎚️ Float Sensor (Primary)
    - ✅ Reliable Detection
    - 🛑 Overflow Prevention
  - 📡 Ultrasonic Sensor
    - 📊 Real-time Monitoring
    - 📈 Level Tracking
  - 🧮 Volume Calculation
    - 💧 Approximate water volume in liters
    - 🔧 Customizable tank capacity

### 🌐 Connectivity
- **📡 WiFi Features**
  - 📱 Smart Device Setup
  - 🔄 OTA Updates
  - ⏰ Auto Time Sync
  - 📊 Remote Monitoring

### ⏱️ Time Management
- **🕒 RTC Features**
  - 📅 Scheduling
  - ⚡ Power Monitoring
  - 🔄 Auto Recovery
  - 📊 Time Tracking

### 📊 Data Analytics
- **☁️ Cloud Integration**
  - 📈 Google Sheets Logging
  - 📊 Performance Analytics
  - 📉 Usage Statistics
  - 🔍 System Diagnostics

> 💡 **Pro Tips:**
> - 🔧 Configure sensors based on your setup
> - ⚙️ Adjust thresholds for optimal performance
> - 📱 Use WiFi features for remote monitoring
> - 🔄 Keep firmware updated for best results

<div align="center">
<table>
<tr>
<td align="center">
<img src="https://img.shields.io/badge/🔒_Safety-Enabled-success?style=for-the-badge" alt="Safety"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/📊_Monitoring-Real--time-blue?style=for-the-badge" alt="Monitoring"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/⚡_Power-Managed-orange?style=for-the-badge" alt="Power"/>
</td>
</tr>
</table>
</div>

<div style="background-color: #F5EEF8; padding: 20px; border-radius: 10px; margin-top: 20px;">

## 📊 Data Logging System

<div align="center">
<img src="https://img.shields.io/badge/🌐_Cloud_Logging-Active-success?style=for-the-badge" alt="Cloud Logging"/>
</div>

### 📡 Overview
This project offers two methods for logging pump data to Google Sheets:
1. 🌟 Using PythonAnywhere as middleware (Recommended, Advanced)
2. 🔗 Direct Google Sheets integration (Comparatively Simpler, Not Recommended)

### Method 1: Using PythonAnywhere 🚀
#### 🔧 Setup Process
1. **🌐 Google Cloud Setup**
   - Follow [this guide](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/) up to step 2
   - 🔑 Obtain necessary Google Cloud credentials
   - 📊 Set up your Google Sheet for data reception

2. **⚙️ PythonAnywhere Configuration**
   - 🔄 Deploy provided Flask code to PythonAnywhere
   - 📂 Code available in `/resources` folder
   - 📡 Handles JSON data from ESP32
   - 📝 Manages Google Sheets communication

3. **📊 Data Collection**
   - ⏱️ Records fill time duration
   - 📈 Logs sensor parameters
   - 📊 Tracks system performance
   - 💾 Stores historical data

### Method 2: Direct Integration ⚡
#### ℹ️ Important Notes
- 📂 Located in `/resources` folder
- 💻 Requires different partition scheme
- 🔧 Higher memory usage
- 📋 Check header comments for configuration

### 📈 Logged Parameters
<table>
<tr>
<td align="center">⏱️ Duration</td>
<td align="center">💧 Water Level</td>
<td align="center">⚡ Power Usage</td>
<td align="center">🔄 System Status</td>
</tr>
</table>

> 💡 **Tip:** Choose Method 1 for better memory management and system stability. Use Method 2 only if you need direct integration and have configured the appropriate partition scheme.

### Data Analysis
- Track system efficiency
- Monitor fill patterns
- Identify potential issues
- Analyze performance trends
</div>

<div style="background-color: #FADBD8; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⚡ What's New
<details>
<summary style="cursor: pointer; font-weight: bold;">V1.2.2</summary>

1. **Water Volume Display**: 
   - Approx. Water available in Litres is live now.
</details>
</div>

<div style="background-color: #F5F5F5; padding: 20px; border-radius: 10px; margin-top: 20px;">

## ⚙️ Calibration & Setup

<div align="center">
<img src="https://img.shields.io/badge/🔧_Setup-Required-yellow?style=for-the-badge" alt="Setup Required"/>
</div>

### 📡 Core Sensors
<table>
<tr>
<td align="center">
<h4>⚡ Current Sensor</h4>
SCT013
</td>
<td align="center">
<h4>💧 Float Sensor</h4>
Water Level
</td>
<td align="center">
<h4>📏 Distance Sensor</h4>
Ultrasonic
</td>
</tr>
</table>

### 🔌 Current Sensor Calibration

#### 1️⃣ Initial Setup
```
📊 Default Value: 27
🔧 Adjustable via program
📈 Reference: Home electricity meter
🎯 Goal: Match readings
```

#### 2️⃣ Expected Readings (1HP Pump)
<table>
<tr>
<td align="center">⚡ Voltage</td>
<td align="center">📊 Current</td>
</tr>
<tr>
<td align="center">220-240V AC</td>
<td align="center">2.6A - 3.3A</td>
</tr>
</table>

#### 3️⃣ Calibration Steps
```
1. 🏃 Run pump normally
2. 📊 Compare readings
3. 🔧 Adjust values
4. ✅ Verify results
```

### 🔧 Tank Volume Configuration
To enable the approximate water volume display:
1. Locate the `TANK_VOLUME` definition in the code.
2. Set its value to your tank's capacity in liters.
3. Example: `#define TANK_VOLUME 1000 // for a 1000-liter tank`

> 💡 **Note:** The accuracy of the volume estimation depends on the precision of your ultrasonic sensor calibration and the `TANK_VOLUME` setting.

### ⚡ Important Notes
- **🔌 Voltage Protection:** External voltage protection is recommended
- **📊 Current-Voltage Relationship:**
  - 📈 Current increases when voltage drops
  - 📉 Current decreases when voltage rises
  - 🔍 This relationship helps detect power quality issues

### 🛡️ Safety Recommendations
<table>
<tr>
<td align="center">
<img src="https://img.shields.io/badge/🔒_Protection-Required-red?style=for-the-badge" alt="Protection Required"/>
</td>
<td align="center">
<img src="https://img.shields.io/badge/📊_Monitoring-Active-blue?style=for-the-badge" alt="Monitoring Active"/>
</td>
</tr>
</table>

1. 🛡️ Always have external circuit protection
2. 📊 Monitor initial readings carefully
3. 📝 Document your calibration values
4. 🔄 Perform periodic calibration checks

> 💡 **Pro Tip:** The relationship between current and voltage provides indirect power quality monitoring even without dedicated voltage sensors.

<table>
<tr>
<td align="center">
<h4>⚠️ Safety First</h4>
- External Protection<br>
- Regular Checks<br>
- Documentation
</td>
<td align="center">
<h4>📊 Monitoring</h4>
- Track Readings<br>
- Record Changes<br>
- Verify Values
</td>
<td align="center">
<h4>🔧 Maintenance</h4>
- Regular Calibration<br>
- Keep Records<br>
- Update Settings
</td>
</tr>
</table>
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

</div>

<div align="center" style="margin-top: 40px; padding: 20px; background: linear-gradient(45deg, #FF6B6B, #4ECDC4, #45B7D1, #6A0572); border-radius: 10px; color: white;">
  <h2 style="margin-bottom: 10px;">Made with ❤️ for the IoT Community</h2>
  <p style="font-size: 1.2em;">Empowering smart homes, one pump at a time! 🏠💧</p>
  <div style="margin-top: 20px;">
    <a href="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller" style="text-decoration: none; background-color: rgba(255,255,255,0.2); color: white; padding: 10px 20px; border-radius: 5px; margin: 0 10px;">⭐ Star Us on GitHub</a>
    <a href="https://github.com/KamadoTanjiro-beep/Advanced-Water-Pump-Controller/issues" style="text-decoration: none; background-color: rgba(255,255,255,0.2); color: white; padding: 10px 20px; border-radius: 5px; margin: 0 10px;">🐛 Report Issues</a>
  </div>
</div>
