# M5Stack Cardputer (original Pigtail target)

Reference notes on the **M5Stack Cardputer** — the hardware the Pigtail firmware
in this repo was originally written for. We are forking Pigtail to run on a
Cheap Yellow Display (CYD); this document records the original device so the
porting effort has an accurate baseline.

> Scope note: this covers the **original Cardputer (v1 / v1.1) built around the
> M5StampS3**, which is the board Pigtail targets (PlatformIO board
> `m5stack-stamps3`). M5Stack also sells a newer **Cardputer-Adv** (2025/2026)
> with different audio (ES8311 codec, NS4150B amp), a larger battery, and an
> improved antenna — that is **not** the device this firmware targets and is
> mentioned only to avoid confusion.

---

## 1. Overview

The Cardputer is a card-sized, battery-powered handheld computer built around
the **M5StampS3** module (an ESP32-S3 "stamp" core). It ships as a small QWERTY
"card" with a tiny color display, a physical keyboard, a speaker, a microphone,
an IR emitter, a microSD slot, and a removable base that holds the larger
battery and a Grove expansion port.

- Form factor: roughly credit-card sized handheld. M5Stack lists dimensions of
  **84.0 x 54.0 x 19.7 mm** for the assembled unit.
- Core module: **M5StampS3**, a self-contained ESP32-S3 module with USB-C, an
  RGB status LED, and a programmable button, that plugs into the Cardputer body.
- Intended use: pocket dev/experimentation platform, CTF/RF tinkering, retro
  handheld projects.

---

## 2. SoC — ESP32-S3 (via M5StampS3)

| Property | Value |
|---|---|
| Module | M5StampS3 |
| SoC | Espressif **ESP32-S3FN8** |
| CPU | Xtensa **LX7** dual-core, up to **240 MHz** |
| Flash | **8 MB** (in-package, the `FN8` suffix) |
| PSRAM | **None** on the original StampS3 (see note) |
| SRAM | 512 KB internal (ESP32-S3 family spec) |
| ROM | 384 KB (ESP32-S3 family spec) |
| Wireless | **Wi-Fi 2.4 GHz 802.11 b/g/n** + **Bluetooth 5 (LE)** |
| USB | Native **USB-OTG** / USB Serial-JTAG (CDC) |

**PSRAM — verified vs. community claim.** This is a common point of confusion.
The original M5StampS3 / Cardputer uses the **ESP32-S3FN8**, which has **8 MB
flash and no PSRAM**. Some third-party product listings (and listings for the
*different* StickS3 product) advertise "8 MB PSRAM"; for the original Cardputer
that is **incorrect**. This repo confirms the no-PSRAM configuration: it builds
with `-DBOARD_HAS_PSRAM=0` and forces NimBLE to allocate from internal RAM only
(`CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL=1`), in `platformio.ini`. Treat
"no PSRAM, 8 MB flash" as the verified configuration for porting.

**Wi-Fi promiscuous mode.** Pigtail relies on the ESP32-S3's promiscuous
RX capability for passive Wi-Fi sniffing — preserved on any ESP32-S3 target,
so this carries over to a CYD that also uses an ESP32 family chip.

---

## 3. Display

| Property | Value |
|---|---|
| Panel | **1.14" IPS TFT** |
| Resolution | **240 x 135** px |
| Controller | **ST7789 (ST7789V2)** |
| Driver stack | **M5GFX / LovyanGFX** |
| Orientation in Pigtail | landscape, 240 wide x 135 tall |

The Cardputer's display is driven through M5Stack's **M5GFX** layer, which is
built on **LovyanGFX**. Pigtail accesses it as `M5Cardputer.Display` and treats
it as 240x135 landscape (`d.width()` == 240, `d.height()` == 135 in
`src/main.cpp`). Pigtail renders into one off-screen 4bpp sprite and pushes it
to avoid flicker (see `src/UIGrid.cpp`).

> Porting note: a CYD is typically a **320x240 ILI9341/ST7789** panel — larger
> and a different aspect ratio than the Cardputer's 240x135, so layout code in
> `UIGrid` will need rework.

---

## 4. Keyboard

| Property | Value |
|---|---|
| Type | Physical **QWERTY** membrane keyboard |
| Layout | **56 keys, 4 x 14 matrix** |
| API | M5Cardputer `Keyboard_Class` (`M5Cardputer.Keyboard`) |

The keyboard is a scanned key matrix exposed through M5Stack's
**`Keyboard_Class`**. Pigtail's usage (`src/main.cpp`, `src/UIGrid.cpp`):

- `M5Cardputer.Keyboard.begin()` — init (in `setup()`).
- `M5Cardputer.update()` then `Keyboard.isChange()` / `Keyboard.isPressed()`
  to gate input each loop.
- `Keyboard_Class& kb` is passed into `UIGrid::handleKeyboard()` /
  `UIGrid::pollLongPress()`.
- `kb.keysState()` returns a struct with modifier/special flags (e.g.
  `ks.space`).
- `kb.isKeyPressed('w')`, `kb.isKeyPressed(';')`, etc. test individual keys.
  Pigtail maps `;`/`.`/`,`/`/` to up/down/left/right navigation and uses
  letter keys for actions (watch, ignore, KML, etc.).

> Porting note: a CYD has **no keyboard** (just a resistive/capacitive
> touchscreen and a few buttons). All `Keyboard_Class` usage in `main.cpp` and
> `UIGrid.cpp` must be replaced with touch/button input on the CYD fork.

---

## 5. Audio

| Property | Value |
|---|---|
| Speaker | **1 W, 8 Ω**, driven via I2S (NS4168 amplifier) |
| Microphone | MEMS mic (SPM1423) — not used by Pigtail |
| API | `M5Cardputer.Speaker` |

Pigtail uses only the speaker, for UI feedback tones:
`M5Cardputer.Speaker.tone(freq, ms)` — e.g. the boot startup sweep/arpeggio in
`playStartupSound()` (`src/main.cpp`).

> Porting note: most CYD boards have **no speaker/amp** (some have a small
> buzzer pad). The `Speaker.tone()` calls will need to be stubbed or remapped.

---

## 6. Power / battery

| Property | Value |
|---|---|
| Battery | **120 mAh** (in the StampS3 body) **+ 1400 mAh** (in the base) |
| API | `M5Cardputer.Power` |
| Charging | over USB-C |

Pigtail reads the battery state of charge with
`M5Cardputer.Power.getBatteryLevel()` (returns 0–100) for the on-screen battery
indicator (`src/UIGrid.cpp`). Note: `M5Cardputer.Power.isCharging()` is present
in M5's API but is **commented out** in Pigtail.

> Porting note: most CYDs have **no battery / no fuel gauge** (USB powered). The
> battery indicator and `Power` calls will need to be removed or stubbed.

---

## 7. Storage

Two independent storage backends are used, and **both are referenced in the
firmware**:

### microSD (SPI)

Pin assignment **as used by this repo** (`src/main.cpp`):

| Signal | GPIO |
|---|---|
| CS   | **12** (`SD_CS`, `GPIO_NUM_12`) |
| MOSI | **14** (`SD_MOSI`) |
| MISO | **39** (`SD_MISO`) |
| SCK  | **40** (`SD_SCK`) |

Initialized via `SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS)` then
`SD.begin(GPIO_NUM_12, SPI, 25000000, "/sd", 1)` (25 MHz, mount point `/sd`).
The SD card is required for **KML export** (`pt_watchlist.kml`).

### SPIFFS (internal flash)

`SPIFFS.begin(true)` (auto-format on failure). Used for persistent
watchlist/ignorelist JSON (`/pt_watchlist.json`, `/pt_ignorelist.json`) so tags
survive reboots (`src/DeviceTracker.cpp`).

> Porting note: CYD boards usually have a microSD slot too, but on **different
> pins** — the `SD_CS/MOSI/MISO/SCK` constants in `main.cpp` will need updating.
> SPIFFS is independent of board pins and carries over unchanged.

---

## 8. GPS — CAP LoRa868 add-on (NOT built in)

The Cardputer has **no onboard GNSS**. Pigtail's GPS support targets M5Stack's
**CAP LoRa868** add-on module, which carries a GPS receiver, attached over the
Cardputer's expansion port.

Configuration **as used by this repo** (`src/main.cpp`, `src/GNSSModule.cpp`):

| Property | Value |
|---|---|
| UART | **UART2** (`HardwareSerial(2)`) |
| RX (ESP32 receives) | **GPIO 15** |
| TX (ESP32 transmits) | **GPIO 13** |
| Baud | **115200** |
| Format | SERIAL_8N1 |
| Parser | TinyGPSPlus |

Call site: `gnss_begin(115200, 15, 13)` → `GNSSModule::begin()` opens
`HardwareSerial(2)` at 115200 8N1 on RX=15 / TX=13. Pigtail uses GPS fixes for
its segment/displacement logic; when no fix is available it falls back to
AP-fingerprint segmentation.

Note: in `setup()` Pigtail also does `pinMode(5, INPUT_PULLUP)` to "disable LoRa
GPIO5 to avoid conflicts" — a side effect of the LoRa868 add-on sharing pins.

> Porting note: the GPS add-on is external and UART-based, so it could in
> principle attach to a CYD on any free UART pins; only the pin numbers and the
> LoRa-conflict handling would change.

---

## 9. GPIO / expansion

- **M5StampS3 GPIO**: the StampS3 module breaks out **23 GPIO** of the ESP32-S3
  (G0–G15, G39–G44, G46).
- **Grove port**: **1 x HY2.0-4P** Grove connector (I2C / UART capable) on the
  base — used for the GPS/LoRa add-on and other Grove peripherals.
- Buttons: a **Reset** button and a **User/programmable** button (G0). Pigtail
  configures G0 with `pinMode(0, INPUT_PULLUP)`.
- IR emitter is present on the Cardputer (not used by Pigtail).

---

## 10. USB / flashing

- Connector: **USB-C** on the M5StampS3.
- Interface: ESP32-S3 **native USB-OTG**, exposed as **USB Serial/JTAG (CDC)**.
- Pigtail enables CDC-on-boot in `platformio.ini`
  (`-DARDUINO_USB_CDC_ON_BOOT=1`, `-DARDUINO_USB_MODE=1`) and uses
  `Serial` at **115200** baud for the console (`Serial.begin(115200)`).
- Build / flash (from `CLAUDE.md` / `platformio.ini`, env `m5stack-stamps3`):
  - `pio run -e m5stack-stamps3` (build)
  - `pio run -e m5stack-stamps3 -t upload` (flash over USB)
  - `pio device monitor -b 115200` (serial console)
- Upload speed configured at 460800; monitor at 115200.

---

## 11. How Pigtail uses each subsystem

| Subsystem | API / pins | Where used |
|---|---|---|
| Core init | `M5.begin()`, `M5Cardputer.begin(cfg, true)` | `src/main.cpp` |
| Display (240x135 ST7789 / M5GFX) | `M5Cardputer.Display` | `src/main.cpp` (splash), `src/UIGrid.cpp` (all rendering) |
| Keyboard (QWERTY matrix) | `M5Cardputer.Keyboard`, `Keyboard_Class`, `isChange/isPressed/isKeyPressed/keysState` | `src/main.cpp` (poll), `src/UIGrid.cpp` (`handleKeyboard`, `pollLongPress`) |
| Speaker | `M5Cardputer.Speaker.tone()` | `src/main.cpp` (`toneMs`, `playStartupSound`) |
| Battery | `M5Cardputer.Power.getBatteryLevel()` | `src/UIGrid.cpp` |
| microSD (CS12/MOSI14/MISO39/SCK40) | `SPI.begin()`, `SD.begin()` | `src/main.cpp` (`initStorage`); KML write in `src/DeviceTracker.cpp` |
| SPIFFS | `SPIFFS.begin()` | `src/main.cpp`; watchlist/ignorelist I/O in `src/DeviceTracker.cpp` |
| GPS (CAP LoRa868, UART2 RX15/TX13 @115200) | `gnss_begin(115200,15,13)`, `HardwareSerial(2)`, TinyGPSPlus | `src/main.cpp`, `src/GNSSModule.cpp` |
| Wi-Fi promiscuous sniff | `esp_wifi_set_promiscuous_rx_cb` + channel hop | `src/DeviceTracker.cpp` |
| BLE scan | NimBLE `NimBLEScanCallbacks` | `src/DeviceTracker.cpp` |
| G0 / LoRa pins | `pinMode(0/5, INPUT_PULLUP)` | `src/main.cpp` |
| Serial console | `Serial.begin(115200)` (USB-CDC) | `src/main.cpp` |

---

## Verified vs. community claims (summary)

- **Verified (M5Stack official docs + this repo):** ESP32-S3FN8, 8 MB flash,
  Xtensa LX7 dual-core @240 MHz, 1.14" 240x135 ST7789V2 IPS, 56-key (4x14)
  QWERTY, 1 W/8 Ω speaker (NS4168), SPM1423 mic, 120 mAh + 1400 mAh battery,
  HY2.0-4P Grove, USB-C / USB-Serial-JTAG, 84 x 54 x 19.7 mm. SD pins
  CS12/MOSI14/MISO39/SCK40 and GPS UART2 RX15/TX13 @115200 are what this
  firmware actually configures.
- **Community / listing claims to distrust:** "8 MB PSRAM" on the original
  Cardputer (it has **no PSRAM**; that figure appears to come from StickS3 or
  the Adv variant). Battery/charging details vary between v1, v1.1, and Adv.
- **Variant caution:** the **Cardputer-Adv** (2025/2026) changes the audio
  subsystem (ES8311 + NS4150B), battery (1750 mAh), and antenna — not this
  firmware's target.

---

## Sources

- [M5Stack Cardputer — official docs (docs.m5stack.com)](https://docs.m5stack.com/en/core/Cardputer)
- [M5Stack Stamp-S3 module — official docs (docs.m5stack.com)](https://docs.m5stack.com/en/core/StampS3)
- [M5Stack Cardputer with M5StampS3 v1.1 — M5Stack store (EOL)](https://shop.m5stack.com/products/m5stack-cardputer-with-m5stamps3-v1-1)
- [M5Stack Cardputer-Adv (ESP32-S3) — M5Stack store](https://shop.m5stack.com/products/m5stack-cardputer-adv-version-esp32-s3)
- [Cardputer-Adv coverage — CNX Software](https://www.cnx-software.com/2025/10/23/m5stack-cardputer-adv-esp32-s3-computer-gains-improved-antenna-larger-1750-mah-battery-es8311-audio-codec/)
- [M5Stack Cardputer overview — Luis Llamas](https://www.luisllamas.es/en/m5stack-cardputer/)
- [M5Stack Card Computer review — Raspberry Pi Official Magazine](https://magazine.raspberrypi.com/articles/m5stack-card-computer-review)
- This repository: `platformio.ini`, `src/main.cpp`, `src/UIGrid.cpp`, `src/DeviceTracker.cpp`, `src/GNSSModule.cpp`, `CLAUDE.md`.
