# IoT-Based Smart Water Level Monitoring and Automated Pump Control System

An ESP32 water level monitor with non-contact ultrasonic sensing, a five-state level indicator, remote monitoring through Blynk IoT, and an autonomous shut-off interlock that prevents tank overflow without operator attention.

![Platform](https://img.shields.io/badge/platform-ESP32-000000)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Cloud](https://img.shields.io/badge/cloud-Blynk%20IoT-2ECC71)
![Simulation](https://img.shields.io/badge/simulated-Wokwi-FF6B00)
![License](https://img.shields.io/badge/license-MIT-blue)

> Minor project, Department of Information and Communication Engineering, Noakhali Science and Technology University.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Live Simulation](#live-simulation)
- [Hardware](#hardware)
- [Wiring](#wiring)
- [Critical Wiring Notes](#critical-wiring-notes)
- [Getting Started](#getting-started)
- [How It Works](#how-it-works)
- [Repository Structure](#repository-structure)
- [Troubleshooting](#troubleshooting)
- [Known Limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Report](#report)
- [Authors](#authors)
- [License](#license)

---

## Overview

Overhead water tanks are still managed almost entirely by eye. Run the pump too long and the tank overflows down the side of the building; run it against an empty source and the motor burns out dry. Neither failure announces itself until the damage is underway.

This project replaces the guesswork with a sub-2,300 BDT node. An HC-SR04 measures the air gap above the water without ever touching it, an ESP32 converts that to a depth and classifies it into five states, and the result appears in three places at once — a 16×2 LCD at the tank, a four-LED bar, and a Blynk dashboard on your phone. When the tank reaches its full threshold the firmware opens the relay itself, so overflow protection does not depend on anyone being awake.

The design was validated in the [Wokwi simulator](#live-simulation) before a single component was connected, which caught five logic and pin-allocation defects at zero component risk.

## Features

- **Non-contact sensing** — no electrodes in the water, so no corrosion, no biofouling, no contamination risk
- **Five-state classification** — Empty, Very Low, Low, Medium, Full, driven by percentage-of-depth thresholds
- **Autonomous shut-off interlock** — pump stops at the full threshold and the dashboard switch resynchronises automatically
- **Works offline** — LCD and LED indication are fully independent of the network; the interlock runs locally, so overflow protection survives an internet outage
- **Self-healing display** — I2C address auto-discovery, full-row redraws, and scheduled reinitialisation recover the LCD from corruption without a manual power cycle
- **Non-blocking firmware** — timer-driven measurement with a bounded acquisition timeout, so the network stack is never starved
- **Fully simulated** — reproducible in a browser with no hardware required

## Live Simulation

**▶ [Run the simulation on Wokwi](https://wokwi.com/projects/471811774070104065)**

No installation, no hardware. The simulated HC-SR04 exposes its distance as a draggable parameter, so you can sweep the full tank range and watch the states, LEDs, LCD and relay respond in real time.

> **Note on fidelity:** Wokwi models a component's intended *function*, not its physical limits. The simulated sensor has no dead zone and simulated pins have no voltage rating. See [Known Limitations](#known-limitations) — this distinction matters and is the subject of Chapter 6.5 of the report.

## Hardware

| Component | Spec | Qty |
|---|---|---:|
| ESP32 DevKit V1 | 30-pin, ESP32-WROOM-32 | 1 |
| HC-SR04 ultrasonic sensor | 2–400 cm, 5 V | 1 |
| 16×2 LCD + I2C backpack | HD44780 + PCF8574 | 1 |
| 5 V relay module | SRD-05VDC-SL-C, opto-isolated | 1 |
| LEDs | 5 mm | 4 |
| Resistors | 180 Ω (LEDs) | 4 |
| Resistors | 1 kΩ + 2 kΩ (Echo divider) | 1 each |
| DC water pump | Submersible mini pump + tube | 1 |
| 18650 Li-ion + holder | 3.7 V nominal | 2 |
| USB power bank | 5 V regulated | 1 |
| Breadboard | Full size | 2 |
| Jumper wires | M-M, M-F, F-F | — |

**Approximate build cost: 2,254 BDT (~$19 USD)**

## Wiring

| Module | Pin | ESP32 | Notes |
|---|---|---|---|
| HC-SR04 | VCC | **VIN (5 V)** | Ranging degrades at 3.3 V |
| HC-SR04 | Trig | GPIO 12 | Direct — 3.3 V clears the input threshold |
| HC-SR04 | Echo | GPIO 13 | **Via 1 kΩ / 2 kΩ divider** |
| HC-SR04 | GND | GND | |
| I2C LCD | VCC | **VIN (5 V)** | HD44780 contrast needs 5 V |
| I2C LCD | SDA | GPIO 21 | Bus clocked at 50 kHz |
| I2C LCD | SCL | GPIO 22 | |
| I2C LCD | GND | GND | |
| Relay | VCC | VIN (5 V) | Coil supply |
| Relay | IN | GPIO 14 | Active LOW on most blue boards |
| Relay | GND | GND | Coil ground only |
| Relay | COM / NO | *not connected to ESP32* | Isolated pump circuit |
| LED 1 | Anode | GPIO 19 | via 180 Ω |
| LED 2 | Anode | GPIO 18 | via 180 Ω |
| LED 3 | Anode | GPIO 5 | via 180 Ω |
| LED 4 | Anode | GPIO 15 | via 180 Ω — strapping pin, see below |

Pump circuit, entirely separate: `battery + → relay COM`, `relay NO → pump +`, `pump − → battery −`.

## Critical Wiring Notes

**Echo needs a voltage divider.** The HC-SR04 drives Echo to its supply rail, so at 5 V you are feeding 5 V into a pin rated for 3.6 V absolute maximum. The internal clamp diodes make it *appear* to work, which is exactly why this defect is invisible during testing and shows up months later as an intermittent pin. Wire `Echo → 1 kΩ → GPIO 13` and `GPIO 13 → 2 kΩ → GND`.

**Keep the power domains separate.** The relay contacts are isolated from its coil. Only VCC, GND and IN cross from the ESP32 to the relay — the pump's battery must **not** share ground with the controller. Joining them throws away the isolation and lets motor inrush reset the board.

**GPIO 12 and 15 are strapping pins.** GPIO 12 selects the flash regulator voltage at reset; if anything pulls it high at boot the board will not start. Never fit a pull-up on the trigger net. GPIO 15 suppresses the boot ROM log when held low — harmless, but it removes diagnostic output. Move LED 4 to GPIO 23 if you want the boot log back.

**Two breadboards, not one.** A 30-pin ESP32 covers a single breadboard completely, leaving no free rows. Join two boards and straddle the module across the seam.

**Check your pump's voltage.** Two Li-ion cells in series is 7.4 V. If your mini pump is rated 3–6 V, use one cell.

## Getting Started

### 1. Install the toolchain

Arduino IDE 2.x, then add the ESP32 board package via **File → Preferences → Additional Board Manager URLs**:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Then **Tools → Board Manager → esp32 → Install**.

### 2. Install libraries

Via **Library Manager**:

| Library | Author |
|---|---|
| Blynk | Volodymyr Shymanskyy |
| LiquidCrystal I2C | Frank de Brabander |

> ⚠️ Install **LiquidCrystal I2C**, not the stock `LiquidCrystal`. The latter drives the display over 6 parallel wires and has no I2C support.

### 3. Set up Blynk

1. At [blynk.cloud](https://blynk.cloud), create a **template** — hardware `ESP32`, connection `WiFi`
2. Add two **datastreams**:

   | Virtual pin | Name | Type | Range | Widget |
   |---|---|---|---|---|
   | `V0` | Water Level | Integer | 0 – *tank depth* | Gauge |
   | `V1` | Water Pump | Integer | 0 – 1 | Switch |

3. Create a **device** from the template and copy the `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME` and auth token

### 4. Configure the sketch

Open `firmware/water_level_monitor.ino` and fill in the marked lines:

```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxx"
#define BLYNK_TEMPLATE_NAME "Water level monitoring system"

char auth[] = "YOUR_BLYNK_AUTH_TOKEN";
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";
```

The template defines **must** sit above `#include <BlynkSimpleEsp32.h>` — the library reads them at compile time.


### 5. Calibrate

Measure from the sensor face down to the **empty** tank bottom, and set:

```cpp
int MaxLevel = 13;   // your tank depth in cm
```

Update the `V0` datastream maximum in Blynk to match.

### 6. Test the hardware first

Upload `firmware/hardware_test.ino` before the main sketch. It scans for your LCD's real I2C address, blinks each LED by name, clicks the relay, and streams distance readings — with a specific fix printed for every failure. Ten minutes here saves an evening of guessing.

### 7. Upload

Board: **ESP32 Dev Module**. Serial Monitor at **115200**.

- Upload fails to connect? Hold **BOOT** while the IDE prints "Connecting…"
- Never joins WiFi? Your network is **5 GHz** — the ESP32 radio is 2.4 GHz only

## How It Works

The sensor measures the *air gap*, so depth is derived first and all thresholds apply to depth — that way a threshold named 90% actually means 90% full.

```
depth = MaxLevel − measured_distance
```

| State | Condition | Depth (13 cm tank) | Sensor reads | LEDs | Pump |
|---|---|---|---|:---:|---|
| **Full** | `depth ≥ Level4` (90%) | 11–13 cm | ≤ 2 cm | 4 | **Forced OFF** |
| **Medium** | `Level3 ≤ depth < Level4` | 9–10 cm | 2–4 cm | 3 | User |
| **Low** | `Level2 ≤ depth < Level3` | 6–8 cm | 4–7 cm | 2 | User |
| **Very Low** | `Level1 ≤ depth < Level2` | 3–5 cm | 7–10 cm | 1 | User |
| **Empty** | `depth < Level1` (25%) | 0–2 cm | > 10 cm | 0 | User |

### The shut-off interlock

On reaching **Full**, the firmware does three things — and the third is what makes it an interlock rather than just a command:

```cpp
digitalWrite(relay, HIGH);            // 1. open the relay
setLine(lineBottom, "Motor is OFF");  // 2. update the local display
Blynk.virtualWrite(V1, 0);            // 3. resync the dashboard switch
```

Without step 3 the app would keep showing the pump as running, presenting an interface that disagrees with the hardware.

Because the interlock is re-evaluated every cycle, it also **overrides a manual command** — switch the pump on while the tank is full and it closes then reopens within one second. That is intentional: overflow protection that the operator can defeat is not protection.

### Display resilience

The LCD subsystem is deliberately defensive, because the HD44780 in 4-bit mode desynchronises permanently if the MCU resets mid-byte:

- I2C address discovered by bus scan, not assumed
- Bus slowed to 50 kHz for long breadboard jumpers
- Both rows buffered and redrawn in full, so corruption self-clears in one second
- Scheduled reinitialisation every 60 s recovers nibble desync with no manual power cycle

## Repository Structure

```
.
├── firmware/
│   ├── water_level_monitor.ino     # main sketch (Wokwi-validated)
│   └── hardware_test.ino           # bring-up diagnostic — run this first
├── simulation/
│   ├── diagram.json                # Wokwi circuit definition
│   └── wokwi-screenshot.png
├── docs/
│   ├── Project_Report.pdf
│   ├── wiring-diagram.png
│   └── figures/
├── .gitignore
├── LICENSE
└── README.md
```

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Backlight on, no characters | Wrong I2C address | Use the address from the startup scan |
| Blank or solid blocks | Contrast | Turn the blue trimmer on the backpack |
| **Recognisable but wrong** characters | 4-bit nibble desync after reset | Full power-cycle; scheduled re-init prevents recurrence |
| Distance always 0 / "no echo" | Trig↔Echo swapped, or sensor on 3.3 V | Trig→12, Echo→13, VCC→VIN |
| Never joins WiFi | 5 GHz network | Switch the AP or hotspot to 2.4 GHz |
| Connects then drops repeatedly | Telemetry flood or blocking `pulseIn` | Confirm timer-driven at 1 s and the 30 ms timeout is set |
| Won't boot with sensor attached | GPIO 12 held high at reset | Remove any pull-up on the trigger net |
| Relay inverted | Board is active-HIGH | Swap `LOW`/`HIGH` in `BLYNK_WRITE(V1)` |
| Board resets when pump starts | Shared power rail | Separate the domains; pump through isolated contacts only |
| **Full state never reached** | Threshold inside the sensor dead zone | Drop `Level4` to 75–80%, or raise the sensor 3 cm |
| Boot log missing | GPIO 15 held low by LED 4 | Expected; move LED 4 to GPIO 23 if needed |
| Works in Wokwi, fails on hardware | Property is electrical/physical, not modelled | Check against the datasheet, not the simulation |

## Known Limitations

- **Dead zone vs. interlock threshold.** At 90% of a 13 cm tank, Full corresponds to a reading of ≤ 2 cm — right at the HC-SR04's blind spot. Reliable in simulation, delayed by 1–4 cycles on hardware. **Fix: set `Level4` to 75–80%, and mount the sensor ≥ 3 cm above the max water line.**
- **No automatic pump start.** The interlock stops the pump but won't start it — only half a closed loop.
- **No hysteresis.** A level resting on a boundary can flicker between states, and would short-cycle the motor once auto-start is added.
- **Fixed speed of sound.** No temperature compensation; expect systematic drift across a wide diurnal swing.
- **Depth, not volume.** Assumes uniform cross-section.
- **No dry-run protection.** Nothing senses the source side.
- **Credentials compiled in.** Changing WiFi means reflashing.
- **Breadboard build.** No enclosure or conformal coating — not suitable for a humid tank environment as-is.

## Roadmap

**Phase 1 — Measurement robustness**
Median-of-5 filtering · threshold hysteresis · DS18B20 temperature compensation · relocate `Level4` clear of the dead zone · tank geometry → litres

**Phase 2 — Closed-loop autonomy**
Auto start on low threshold · source-side dry-run protection · max-runtime watchdog · push alerts · local fallback thresholds · WiFiManager captive portal · physical override button

**Phase 3 — Multi-node deployment**
MQTT with `building/floor/tank` topics · LoRa for out-of-coverage reservoirs · aggregated Grafana dashboard · InfluxDB time-series · OTA updates · TP4056 solar charging

**Phase 4 — Predictive**
Consumption forecasting · leak detection from anomalous draw-down · predictive maintenance from fill-rate trends · TensorFlow Lite Micro on-device inference · municipal supply-window scheduling

**Beyond domestic use:** irrigation reservoirs · riverine flood early warning · fuel and chemical tanks · wastewater monitoring

## Report

The full academic report — literature review, methodology, simulation validation, quantitative results, cost analysis and roadmap — is in [`docs/Project_Report.pdf`](docs/Project_Report.pdf).

Chapters most worth reading:

- **5.4 — Engineering Challenges.** Three integration faults, each of which initially pointed at the wrong subsystem
- **6.5 — Limits of Simulation Fidelity.** Why the threshold that worked perfectly in simulation is the least reliable part of the physical build

## Authors

| Name | Roll |
|---|---|
| [Student Name 1] | [Roll No.] |
| [Student Name 2] | [Roll No.] |
| [Student Name 3] | [Roll No.] |

**Supervisor:** [Supervisor Name], [Designation]
Department of Information and Communication Engineering
Noakhali Science and Technology University

## Acknowledgements

- [SriTu Hobby](https://srituhobby.com/how-to-make-a-water-level-monitoring-system-with-esp32-board-and-blynk/) — the reference tutorial this project started from
- [Wokwi](https://wokwi.com) — browser-based simulation
- [Blynk](https://blynk.io) — IoT platform

## License

Released under the MIT License. See [LICENSE](LICENSE).
