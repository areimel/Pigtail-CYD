# Pigtail-CYD — Build Plan

Living checklist for porting **Pigtail** (M5Stack Cardputer, ESP32-S3) to the **Cheap Yellow
Display ESP32-2432S028R** (original ESP32-WROOM). See `docs/PRD.md` for requirements and
`docs/devices/CYD-ESP32-2432S028R.md` for the hardware reference.

## Decisions
- **Board:** ESP32-2432S028R only (no multi-board HAL).
- **Strategy:** clean CYD-only fork — remove M5Cardputer entirely; central pins in `src/BoardConfig.h`.
- **Input:** touchscreen — tap to select/open detail + on-screen action bar; long-press for destructive actions.
- **GPS:** dropped — movement segmentation via Wi-Fi AP fingerprint (`EnvFingerprint`).

## What ports as-is vs. changes
- **Reuse unchanged:** `DeviceTracker`, `Track.h`, `BleTracker/BleGlasses/BleFlock`, `MacPrefixes.h`,
  `Icons/Icon/Logo/Colors`, `RetroAvatar`, `MarkovNameGenerator/Names`, `DeterministicRng`,
  Wi-Fi promiscuous + NimBLE scan core.
- **Replace:** display device, input, audio, SD pins, board env / partitions.
- **Remove:** `GNSSModule`, battery indicator, all `M5Cardputer`/`M5.` calls.

---

## Phase 0 — Documentation & scaffolding
- [ ] `docs/devices/M5Cardputer.md` — original device reference
- [ ] `docs/devices/CYD-ESP32-2432S028R.md` — target device reference
- [ ] `docs/PRD.md` — product requirements
- [ ] `PLAN.md` — this file
- [ ] `src/BoardConfig.h` — central pins/dimensions (DRY single source of truth)
- [ ] Update `CLAUDE.md` to describe the CYD target
- **Success:** docs reviewed; `BoardConfig.h` compiles when included.

## Phase 1 — Build system & display bring-up
- [ ] Rewrite `platformio.ini`: `board = esp32dev`, `mcu = esp32`, 4 MB flash, `flash_mode = dio`,
      115200 monitor. Drop USB-CDC / M5 StampS3 flags. Keep `-Wl,-zmuldefs`, NimBLE internal-mem,
      `-std=gnu++2a`, `-Os`.
- [ ] `lib_deps`: remove `m5stack/M5Cardputer` + `mikalhart/TinyGPSPlus`; add `lovyan03/LovyanGFX`.
- [ ] Add `partitions-4mb.csv` (nvs + app ~2 MB + SPIFFS ~1.5 MB); confirm binary fits.
- [ ] `src/Display.h/.cpp`: `class LGFX : public lgfx::LGFX_Device` for ESP32-2432S028R
      (ILI9341 default + `-DCYD_PANEL_ST7789` switch, XPT2046 touch in same device); global `LGFX lcd;`.
- [ ] Port `drawSplashScreen()` + strip M5 init from `setup()`.
- [ ] Flash over USB (document CH340 driver); apply invert/BGR fix if needed.
- **Success:** CYD boots, splash + version render with correct colors/orientation; serial at 115200.

## Phase 2 — UI rendering port (320×240)
- [ ] Point `UIGrid` at global `lcd`; remove `#include <M5Cardputer.h>` from `UIGrid.h`.
- [ ] Resize sprite to 320×240 4bpp (~38 KB); reflow grid (COLS/ROWS/TILE) for landscape.
- [ ] Keep selection-lock (`syncSelectionToId`) + detail view.
- [ ] Remove battery indicator (`updateBatteryIfDue`/`drawBatteryIndicator`, `M5.Power`).
- **Success:** grid + detail render at 320×240; no M5 symbols remain in UI code.

## Phase 3 — Sensing core on WROOM + RAM validation
- [ ] Build `DeviceTracker` unchanged; verify Wi-Fi promiscuous + NimBLE coexist on WROOM;
      confirm `-Wl,-zmuldefs` override of `ieee80211_raw_frame_sanity_check` links.
- [ ] Remove GPS usage from `main.cpp` (`gnss_begin`/`snapshot`/`setGpsFix`); delete `GNSSModule`.
- [ ] Measure free heap with Wi-Fi+BLE+sprite live (`PrintHeapTelemetry`); tune
      `MAX_TRACKS`/`MAX_ANCHORS`/UI `_items[]` if thin.
- **Success:** real devices populate the grid; stable free-heap headroom (record the number).

## Phase 4 — Touch input & on-screen controls
- [ ] Read touch via `lcd.getTouch()`; one-time XPT2046 calibration persisted (SPIFFS/NVS).
- [ ] Replace `handleKeyboard`/`pollLongPress(Keyboard_Class&)` with a touch handler routing to the
      existing action methods (open/close detail, watch/ignore, mute, export KML, reset, clear lists).
- [ ] On-screen action bar: tile tap = select; tap detail = open; bar buttons = watch/ignore/export/reset;
      long-press = clear list (existing dual-tone confirm).
- **Success:** every former keyboard control reachable by touch.

## Phase 5 — Peripherals & persistence
- [ ] SD on CYD pins (CS5/MOSI23/MISO19/SCK18); verify touch+SD coexistence (bit-bang touch fallback).
- [ ] SPIFFS watchlist/ignorelist read/write incl. legacy MAC byte-order compatibility (`readWatchlist`).
- [ ] KML export to SD (`/pt_watchlist.kml`) gated on `_sdAvailable`.
- [ ] LEDC tone helper on GPIO26 to replace `M5Cardputer.Speaker.tone()` (or clean stub).
- [ ] (Optional) RGB LED status (mind GPIO4 usage).
- **Success:** watchlist survives reboot; KML exports to SD; UI tones play or are cleanly stubbed.

## Phase 6 — Polish, RAM tuning, CI & release
- [ ] Finalize ILI9341/ST7789 + BGR/invert handling; document variant selection.
- [ ] Backlight PWM brightness; optional LDR auto-dim.
- [ ] CI workflow (fork of `pio-cardputer.yml`) building the CYD env + `esptool merge_bin` + release attach;
      keep `VERSION` extraction working.
- [ ] README rewrite: wiring, flashing (CH340), touch controls, parity matrix, quirks.
- **Success:** CI produces a flashable merged binary; README accurate; hardware smoke test passes.

---

## End-to-end verification
1. `pio run` builds clean; binary fits 4 MB partition table.
2. `pio run -t upload` flashes over USB (CH340 driver installed).
3. `pio device monitor -b 115200`: splash correct; free-heap telemetry healthy.
4. Walk near known devices → appear, score, classify (tracker/glasses/Flock if available).
5. Touch: navigate, open detail, watch/ignore, export KML, long-press to clear a list.
6. Reboot → watchlist/ignorelist persist; SD KML present.
7. CI attaches a merged, flashable binary matching `VERSION`.

## Risks
- **RAM on no-PSRAM WROOM** — central risk; 4bpp sprite mitigates; Phase 3 measures & tunes.
- **Display driver lottery (ILI9341 vs ST7789)** — compile-time switch + BGR/invert handling.
- **Touch + SD shared SPI** — validate coexistence in Phase 5.
