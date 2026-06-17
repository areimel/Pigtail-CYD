# Pigtail-CYD — Product Requirements Document

**Status:** Draft v1
**Last updated:** 2026-06-16
**Target board:** ESP32-2432S028R ("Cheap Yellow Display" / CYD)

---

## 1. Overview / Summary

Pigtail-CYD is a clean fork of [Pigtail](https://github.com/) — passive RF-awareness firmware that observes nearby Wi-Fi and BLE activity, measures how long each device lingers near the user, scores devices for "is someone following me?" suspicion, and renders them on a small display using vendor icons, deterministic retro avatars, and stable human-readable names. It additionally classifies BLE trackers (AirTag, Samsung SmartTag, Tile, Google Find My, Chipolo, PebbleBee, etc.), smart glasses (Meta Ray-Ban, Snap Spectacles, EssilorLuxottica), and Flock Safety surveillance hardware. Where the original targets the M5Stack Cardputer (ESP32-S3 with keyboard and optional GPS), Pigtail-CYD retargets the firmware to the widely available, ~$10 **ESP32-2432S028R** "Cheap Yellow Display," trading the keyboard for a resistive touchscreen and dropping GPS, while preserving the core sensing, scoring, and classification feature set within a no-PSRAM RAM budget.

---

## 2. Goals

- **Feature parity** with Cardputer Pigtail's sensing, scoring, and classification on a commodity ~$10 board.
- **Touch-first UX**: replace all keyboard shortcuts with on-screen touch controls (tap to select/open, persistent action bar, long-press for destructive actions).
- **Fit in RAM** on a no-PSRAM ESP32-WROOM (520 KB SRAM) alongside the Wi-Fi and BLE stacks, with stable free-heap headroom over long runs.
- **Single, clean target**: remove the M5Cardputer dependency entirely; no multi-board abstraction layer.
- **Reliable persistence**: watchlist/ignorelist survive reboots (SPIFFS); KML export to SD card.
- **CI-built flashable binary**, mirroring the original project's release workflow.

---

## 3. Non-Goals

- **No Cardputer support** in this repository. This is a hard fork dedicated to the CYD; the M5Cardputer dependency is removed.
- **No GPS.** No coordinate acquisition, no GPS-based movement segmentation. (KML export remains but carries no GPS coordinates.)
- **No multi-board HAL / abstraction.** Code targets the ESP32-2432S028R directly.
- **No support for other CYD variants** (e.g., capacitive GT911 boards, S3-based CYDs, 2-USB revisions) in v1 — noted as future work.
- **No battery / fuel-gauge UI** — the CYD has no battery management or fuel gauge.

---

## 4. Target Users & Use Cases

**Users**

- Privacy-conscious individuals wanting low-cost situational awareness of nearby RF devices.
- Security researchers and educators demonstrating BLE/Wi-Fi tracking surfaces.
- Journalists and at-risk individuals assessing whether a tracker or device is persistently following them.

**Use cases**

- Detect a BLE tracker (AirTag/SmartTag/Tile/etc.) that lingers across multiple locations — a classic "unwanted tracker" pattern.
- Notice surveillance-relevant hardware (Flock Safety devices, smart glasses) in the environment.
- Watch a specific device over time; ignore known-friendly devices; export findings (KML) for later review.

**Ethical & legal-use note**

> Pigtail-CYD performs **passive observation only** — it listens to over-the-air management/advertising traffic that devices already broadcast publicly. It does not connect to, deauthenticate, jam, decrypt, or interfere with any device or network. It is intended for **personal counter-surveillance awareness, research, and education on equipment and networks you are authorized to observe.** Users are responsible for complying with all applicable local laws regarding radio monitoring and privacy.

---

## 5. Functional Requirements

| # | Requirement | Priority |
|---|---|---|
| FR-1 | Passively scan nearby **Wi-Fi** activity via promiscuous-mode RX with channel hopping, capturing clients and access points. | Must |
| FR-2 | Passively scan nearby **BLE** advertisements via NimBLE scan callbacks. | Must |
| FR-3 | Track per-device **lingering** over bucketed time windows and compute a **suspicion score** reflecting consistency of presence and movement coverage. | Must |
| FR-4 | Derive **movement segmentation** from **Wi-Fi AP-fingerprint change** (no GPS), feeding the coverage component of the score. | Must |
| FR-5 | Classify **BLE trackers** (Apple Find My/AirTag, Samsung SmartTag, Tile, Google Find My, Chipolo, PebbleBee, etc.). | Must |
| FR-6 | Classify **smart glasses** (Meta Ray-Ban, Snap Spectacles, EssilorLuxottica) by company ID + name. | Must |
| FR-7 | Classify **Flock Safety** hardware (mfg ID `0x09C8` → Raven service UUIDs `0x3100`–`0x3500` → name → OUI `B4:1E:52`). | Must |
| FR-8 | Render a **grid screen** of device tiles (vendor icon / avatar modes) and a **detail screen** for a selected device. | Must |
| FR-9 | Generate a **deterministic retro avatar** and a **stable human-readable name** per device, seeded from its MAC. | Must |
| FR-10 | **Vendor lookup** from compiled MAC-prefix table. | Must |
| FR-11 | **Watchlist** and **ignorelist** management, persisted as JSON on **SPIFFS** across reboots. | Must |
| FR-12 | **KML export** of the watchlist to the **SD card** (without GPS coordinates). | Must |
| FR-13 | **Touch controls**: tap a tile to select / open detail; persistent on-screen **action bar** (Watch / Ignore / Export / Reset); **long-press** for destructive actions (clear watchlist / clear ignorelist). | Must |
| FR-14 | Render the entire UI flicker-free via an off-screen sprite that is pushed to the display each frame. | Must |
| FR-15 | Maintain selection lock to a device across snapshot reshuffles so the cursor/selection does not jump. | Should |
| FR-16 | Surface basic environment status (e.g., scan activity, device counts) in the UI. | Should |
| FR-17 | Expose firmware **version** in a single source location for CI release tagging. | Must |

---

## 6. Feature Parity Matrix vs Cardputer Pigtail

| Feature | Cardputer (original) | CYD v1 | Notes |
|---|---|---|---|
| Wi-Fi passive scanning | Yes | Yes | Promiscuous RX + channel hop; ESP32-WROOM radio. |
| BLE scanning + classification | Yes | Yes | NimBLE; trackers / glasses / Flock preserved. |
| Lingering + suspicion scoring | Yes | Yes | Windowed scoring preserved. |
| Movement segmentation | GPS displacement (~50 m) with AP-fingerprint fallback | **AP-fingerprint only** | GPS dropped; fingerprint change is the sole segment driver. |
| Retro avatars + deterministic names | Yes | Yes | Seeded from MAC. |
| Vendor icons / lookup | Yes | Yes | Compiled MAC-prefix table reused. |
| Watchlist / ignorelist (persisted) | SPIFFS JSON | SPIFFS JSON | Preserve legacy MAC byte-order compatibility on read. |
| KML export | SD card (with GPS coords) | SD card (**no GPS coords**) | Export path retained; coordinates omitted. |
| Input method | Physical keyboard shortcuts | **Touchscreen** (tap + action bar + long-press) | XPT2046 resistive touch. |
| Display | 240 x 135 | **320 x 240** | Larger canvas; UI re-laid out. |
| Battery indicator | Yes (Cardputer fuel gauge) | **Removed** | CYD has no battery management / fuel gauge. |
| Serial console | Native USB-CDC | **USB-serial bridge (CH340/CP2102)** | No native USB-CDC; monitor over the bridge. |
| Onboard RGB LED / LDR / speaker | N/A | Available (optional use) | CYD-specific peripherals; optional UX hooks. |
| Multi-board support | N/A | No | Single target by design. |

---

## 7. Hardware Constraints

**Board: ESP32-2432S028R**

- **MCU:** original ESP32-WROOM (Xtensa dual-core), **NOT** ESP32-S3.
- **Flash:** 4 MB. **RAM:** ~520 KB SRAM. **No PSRAM.**
- **Display:** 320 x 240 SPI TFT — **ILI9341 or ST7789 depending on batch**, with a known **BGR / color-inversion quirk** that must be handled per-panel.
- **Touch:** XPT2046 **resistive** touchscreen (SPI).
- **Peripherals:** onboard RGB LED, LDR light sensor, small speaker (GPIO26), microSD slot.
- **USB:** micro-USB via **CH340/CP2102 USB-serial bridge** — **no native USB-CDC**.

**Key constraints**

1. **RAM budget (the dominant constraint).** On ~520 KB total SRAM, the firmware must simultaneously hold the Wi-Fi stack (~50 KB), NimBLE (~30–40 KB), a 4bpp off-screen sprite (~38 KB for the larger 320 x 240 canvas), and the shared state arrays (tracks + anchors, on the order of ~80–100 KB). State-array sizing (e.g., `MAX_TRACKS`, `MAX_ANCHORS`), sprite color depth, and stack sizes may need tuning to keep stable free-heap headroom. **No PSRAM means no fallback** for large buffers.
2. **Display driver "lottery."** The exact panel (ILI9341 vs ST7789) and color order (BGR / inversion) varies by board batch; the driver/init must accommodate the variant(s) shipped, including the inversion quirk.
3. **Shared SPI bus.** Touch (XPT2046) and microSD share SPI with (and alongside) the TFT; bus arbitration / chip-select discipline is required so touch sampling and SD access (KML export) do not corrupt display traffic.
4. **No native USB-CDC.** Serial console is over the CH340/CP2102 bridge; flashing and monitoring use the bridge rather than ESP32-S3 native USB.
5. **No fuel gauge / battery management** — power/battery UI is removed.

---

## 8. Success Metrics / Acceptance Criteria

- **Boots & renders correctly** on the ESP32-2432S028R, including correct colors on the shipped panel variant (no inverted/BGR artifacts).
- **Detects and classifies real devices**: in a live environment, Wi-Fi and BLE devices appear; at least one real BLE tracker, smart-glasses, or Flock device classifies correctly when present.
- **Touch controls fully functional**: tap-to-select/open works; the action bar (Watch / Ignore / Export / Reset) operates; long-press triggers destructive actions with appropriate confirmation/affordance.
- **Watchlist persists across reboot**: a watched device remains watched after power-cycle (SPIFFS round-trip), including legacy-format reads.
- **KML exports to SD**: pressing Export writes a valid KML file to the SD card (coordinates omitted).
- **Stable free-heap headroom over a long run**: no monotonic heap decline / out-of-memory crash over an extended (multi-hour) session.
- **CI produces a flashable binary**: the build pipeline compiles, merges a bootable image, and attaches it to a release, with the version derived from the single firmware version source.

---

## 9. Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| RAM exhaustion (Wi-Fi + BLE + sprite + state arrays on 520 KB, no PSRAM) | High | High | Tune state-array caps, sprite depth, and task stacks; monitor free heap; budget early and add a heap-watchdog/telemetry path. |
| Display driver/panel variance (ILI9341 vs ST7789, BGR/inversion) | High | Medium | Support the shipped variant(s); make panel/color-order/inversion configurable; document detection or build-time selection. |
| Shared-SPI contention (TFT + touch + SD) corrupting display or SD I/O | Medium | Medium | Strict CS/bus arbitration; serialize SD (KML) access against frame pushes; test touch under concurrent SD writes. |
| Resistive touch accuracy/calibration vs the original keyboard precision | Medium | Medium | Calibration routine; generous tile/hit-target sizing on the 320 x 240 canvas; long-press affordances for destructive actions. |
| Loss of GPS reduces movement-segmentation fidelity | Medium | High (by design) | Rely on AP-fingerprint segmentation; clearly scope KML as coordinate-free; document the tradeoff. |
| No native USB-CDC complicates flashing/monitoring | Low | Low | Use CH340/CP2102 bridge; document driver install and monitor baud in build/flash docs. |
| Removing M5Cardputer dependency introduces regressions in UI/input paths | Medium | Medium | Clean fork with explicit touch UX layer; parity-test against the matrix in §6. |

---

## 10. Out of Scope / Future

- **Larger ESP32-S3-based CYD variants** (more RAM/PSRAM, native USB) — would relax the RAM budget and re-enable native USB-CDC.
- **Capacitive-touch (GT911) CYD boards** and other panel/touch variants.
- **Re-adding optional external GPS** (e.g., a UART GPS module) to restore coordinate-based segmentation and KML coordinates.
- Use of onboard **RGB LED / LDR / speaker** for richer alerting (e.g., audible/visual tracker alerts, auto-brightness).
- Multi-board abstraction, should additional targets be supported later.
