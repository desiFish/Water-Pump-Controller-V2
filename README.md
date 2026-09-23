# Advanced Water Pump Controller

<p>
  <span style="color:#168aad"><b>ESP32</b></span> controller for a water pump or induction motor with local web control, scheduled operation, tank-level monitoring, current protection, RTC timekeeping, OLED status, and OTA updates.
</p>

<p>
  <a href="https://github.com/desiFish/Water-Pump-Controller-V2/releases"><img src="https://img.shields.io/github/v/release/desiFish/Water-Pump-Controller-V2" alt="Latest release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/desiFish/Water-Pump-Controller-V2" alt="GPLv3 license"></a>
  <img src="https://img.shields.io/badge/board-ESP32-168aad" alt="ESP32 board">
  <img src="https://img.shields.io/badge/interface-local%20web%20page-2a9d8f" alt="Local web page">
</p>

> <span style="color:#d97706"><b>Safety:</b></span> This is a hobby and development project. Mains voltage, contactors, relays, pumps, and water are dangerous together. Use correct fuses, earthing, insulation, enclosures, cable sizes, isolation, and a qualified electrician. Do not treat this controller as a fail-safe or life-safety device.

## What It Does

- Controls the pump from a physical button or the local web page.
- Runs up to three independent daily schedule windows.
- Allows each schedule timer to be enabled or disabled.
- Monitors a float switch, ultrasonic tank level, and current sensor when enabled.
- Stops the pump for a full tank, overcurrent, or undercurrent condition.
- Shows status on a 128x64 OLED and a browser dashboard.
- Stores configuration in ESP32 `Preferences` so settings survive restart.
- Provides Wi-Fi Manager mode for first-time network setup.
- Supports local OTA firmware updates through ElegantOTA.
- Provides browser backup and restore of saved settings.
- Reports firmware alerts through a browser-polled error buffer.

<span style="color:#b91c1c"><b>Important:</b></span> Current protection depends on correctly calibrated sensors and configured limits. The relay or contactor hardware must also be rated for the pump.

## Hardware

| Part | Firmware detail |
|---|---|
| DOIT ESP32 DevKit V1 | Main controller; tested target is `esp32doit-devkit-v1` |
| 128x64 SH1106 OLED | I2C address `0x3C` |
| DS1307 RTC | I2C time source |
| SCT013 current sensor | Read through EmonLib |
| Waterproof ultrasonic sensor | Serial 2; RX `16`, TX `17` |
| Float sensor | Analog input `36` |
| Pump relay or contactor driver | Relay output `2` |
| Push button | Input `15` |
| WS2812B LED | Data pin `4` |
| Buzzer | Pin `5` |
| Current sensor input | Pin `39` |

The voltage sensor is currently not used. Use an appropriately rated relay, SSR, or contactor between the ESP32 control circuit and the pump power circuit.

## Software Requirements

- Arduino IDE or Arduino CLI
- ESP32 board support package
- LittleFS data upload support
- ESP32-compatible libraries:
  - `AsyncTCP`
  - `ESPAsyncWebServer`
  - `ElegantOTA`
  - `Adafruit GFX Library`
  - `Adafruit SH110X`
  - `RTClib`
  - `NTPClient`
  - `Adafruit NeoPixel`
  - `EmonLib`
  - `ArduinoJson`

The exact installed library versions should be compatible with the ESP32 core selected in the build environment.

## First Setup

1. Install the ESP32 board package and the libraries listed above.
2. Open `Advanced-Water-Pump-Controller.ino`.
3. Select **DOIT ESP32 DEVKIT V1** or the equivalent ESP32 board.
4. Connect the hardware with power disconnected from the pump circuit.
5. Compile and upload the firmware.
6. Upload the `data/` folder to LittleFS.
7. Open the Serial Monitor at `115200` baud.
8. On first boot, connect to the access point:
   - SSID: `WIFI_MANAGER`
   - Password: `WIFImanager`
9. Open `http://192.168.4.1/wifi` and save the home Wi-Fi credentials.
10. After restart, find the ESP32 IP address in the serial output or router client list.
11. Open `http://<esp32-ip>/settings`.

Example Arduino CLI commands:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 .
arduino-cli upload -p <port> --fqbn esp32:esp32:esp32doit-devkit-v1 .
```

Upload the filesystem using the filesystem uploader supported by your Arduino environment. A firmware upload alone does not replace `data/settings.html` on LittleFS.

## Web Settings Guide

Open `/settings` after the controller joins Wi-Fi.

### Live Status

The page polls the firmware for pump state, tank percentage, ultrasonic distance, current, float state, Wi-Fi state, RSSI quality, and controller time.

The Start/Stop button uses the latest `pumpRunning` value from the live-status response. It does not make a separate status request. Browser commands require confirmation and duplicate clicks are blocked while a command is pending.

### Tank Calibration

- `Tank Empty Distance`: sensor distance when the tank is empty.
- `Tank Full Distance`: sensor distance when the tank is full.

Save both values together. The empty distance must be greater than the full distance. Invalid calibration returns `0%` rather than a useful tank estimate.

### Current Limits

- `Minimum Current`: undercurrent threshold.
- `Maximum Current`: overcurrent threshold.

Current checks are active only when the current sensor is enabled and the limits are valid. Current-based checks are ignored briefly after startup to avoid nuisance shutdown during inrush.

### Features

Enable only the sensors installed and calibrated on the controller: ultrasonic sensor, current sensor, float sensor, Wi-Fi, and automatic run schedule.

### Automatic Schedule

There are three independent timer groups. Each group has an enable checkbox, a pump start time, and a pump stop time.

The page displays times as `HH:MM`. The firmware stores them internally as `HHMM`; for example, `06:15` becomes `615` and `16:15` becomes `1615`.

Disabled timers are ignored by the firmware. A schedule window may cross midnight, for example `22:30` to `06:30`.

Automatic start has a 10-second countdown. Press the physical button during the countdown to cancel it. A full tank or unsafe current condition prevents the pump from starting and creates a browser alert through the firmware error buffer.

### RTC Settings

- **Sync RTC from Wi-Fi** obtains time from the configured NTP server.
- **Sync RTC From Browser Time** sends the browser device time.
- **Set RTC Manually** sends a selected date and time.

NTP synchronization requires working Internet access, not only a local Wi-Fi connection.

### Backup and Restore

- **Backup Settings** downloads a JSON file containing settings returned by the firmware.
- **Restore Settings** validates the project backup format and writes settings section by section.
- Wi-Fi settings are restored last.
- Restore requires confirmation and shows progress animation.

<span style="color:#b45309"><b>Protect backup files:</b></span> they contain the saved Wi-Fi password and should not be committed to a public repository or shared unsecured.

### OTA and Restart

- **OTA Update** opens the ElegantOTA page at `/update`.
- **Restart ESP32** requests a controlled device restart.

Do not remove power during a firmware update.

### Cloud Logging and API Key

The current firmware retains an API key field and logging-related code, but the web logging workflow is not the finished user-facing feature.

<span style="color:#2563eb"><b>Planned:</b></span> cloud logging and API-key behavior will be updated in a future release. Do not depend on the current field for production logging.

## Physical Controls

- Short button interaction opens the pump start/stop confirmation flow.
- Long button interaction opens the device menu.
- The OLED shows pump state, time, sensor readings, tank level, and safety messages.
- The RGB LED and buzzer provide local status and warning feedback.

Automatic operation and browser operation use a separate start flow from the physical button flow. This prevents the scheduled countdown from interfering with manual confirmation logic.

## API Reference

The firmware serves these active endpoints:

| Method | Endpoint | Purpose |
|---|---|---|
| `GET` | `/api/ping` | Basic connectivity check |
| `GET` | `/api/live` | Live sensor and pump status |
| `GET` | `/api/settings` | Read persisted settings |
| `POST` | `/api/settings/section` | Save one settings section |
| `POST` | `/api/pump/control` | Queue pump start or stop |
| `GET` | `/api/error-buffer` | Read and clear firmware alerts |
| `POST` | `/api/rtc/sync` | Set RTC from JSON date/time |
| `POST` | `/api/rtc/update` | Sync RTC from NTP |
| `GET` | `/api/version` | Read firmware and software versions |
| `POST` | `/api/restart` | Request restart |

### Read live status

```bash
curl http://<esp32-ip>/api/live
```

Typical response fields include:

```json
{
  "tankPercent": 75,
  "ultrasonicDistance": 65,
  "liveAmp": 2.45,
  "floatSensor": false,
  "pumpRunning": false,
  "wifiRSSI": -65,
  "wifiConnected": true,
  "dateTime": "2026-09-23 14:30:45"
}
```

### Control the pump

```bash
curl -X POST http://<esp32-ip>/api/pump/control \
  -H 'Content-Type: application/json' \
  -d '{"action":"start"}'
```

Valid actions are `start` and `stop`. A start request is queued and safety-checked after the 10-second countdown.

### Save one section

```bash
curl -X POST http://<esp32-ip>/api/settings/section \
  -H 'Content-Type: application/json' \
  -d '{"section":"tankCalibration","data":{"tankLow":300,"tankFull":100}}'
```

Supported sections are `tankCalibration`, `currentLimits`, `features`, `autoRunSchedule`, `cloudLogging`, and `wifi`.

### Read firmware alerts

```bash
curl http://<esp32-ip>/api/error-buffer
```

The browser polls this endpoint every five seconds and displays returned messages as alerts.

## Safety and Troubleshooting

### Pump will not start

1. Check the live tank and float status.
2. Check that the tank is not full.
3. Check current limits and sensor calibration.
4. Check that the relevant feature toggles are correct.
5. Read the browser alert buffer and OLED message.
6. Confirm the relay or contactor driver is powered and wired correctly.

### Web page does not open

1. Confirm the ESP32 joined the configured Wi-Fi network.
2. Read the assigned IP address from the serial output.
3. Try `/settings` and `/`.
4. Confirm `settings.html` was uploaded to LittleFS.

### RTC update fails

1. Confirm Wi-Fi status is connected.
2. Confirm the network provides Internet/DNS access.
3. Check the RTC wiring and battery.
4. Try browser-time or manual RTC synchronization.

### Sensor readings look wrong

1. Verify common ground and supply voltage.
2. Verify the pin wiring against the table above.
3. Recheck tank calibration.
4. Confirm the sensor is enabled only after it is connected.
5. Keep high-voltage wiring physically separate from sensor wiring.

## Project Files

| Path | Purpose |
|---|---|
| `Advanced-Water-Pump-Controller.ino` | Main ESP32 firmware |
| `data/settings.html` | Browser settings and live-control page |
| `data/wifimanager.html` | First-time Wi-Fi setup page |
| `resource/` | Reference firmware, server experiments, and media |
| `LICENSE` | GNU GPL version 3 license text |

## GNU GPLv3: What You May and Must Do

This project is distributed under the GNU General Public License, version 3. See [LICENSE](LICENSE) for the complete legal text.

### You may

- ✅ Use the software for private, educational, commercial, or modified projects.
- ✅ Study how the firmware and web page work.
- ✅ Modify the source code.
- ✅ Run and distribute your modified version.
- ✅ Sell copies or hardware containing the software.
- ✅ Publish your modifications under GPLv3 when distributing the covered work.

### You must when distributing

- ✅ Include a copy of the GPLv3 license.
- ✅ Preserve existing copyright and warranty notices.
- ✅ Provide the corresponding source code, or a valid written offer where GPLv3 permits one.
- ✅ Mark important modifications and include the date of change where appropriate.
- ✅ Keep the same GPLv3 freedoms for the covered derivative work.
- ✅ Tell recipients about their GPLv3 rights and any applicable warranty disclaimer.

### You must not

- ❌ Add legal or technical restrictions that remove GPLv3 freedoms.
- ❌ Claim that the original author endorses your modified product without permission.
- ❌ Remove copyright, license, or warranty notices.
- ❌ Distribute binaries while withholding the corresponding source code when GPLv3 requires it.
- ❌ Present this project as safety-certified equipment.

GPLv3 grants copyright permissions; it does not automatically grant trademark rights, patent licenses beyond the license terms, or permission to use project branding as an endorsement. For legal questions, consult the license text and qualified legal counsel.

## Development Status

This project is actively evolving. Verify the firmware version, hardware wiring, sensor calibration, and safety behavior before every deployment. Report reproducible issues with the board, firmware version, wiring, logs, and steps to reproduce.

<p>
  <span style="color:#168aad"><b>Firmware:</b> 1.4.3</span> ·
  <span style="color:#2a9d8f"><b>Software:</b> 1.1.0</span> ·
  <span style="color:#6b7280"><b>License:</b> GPLv3</span>
</p>
