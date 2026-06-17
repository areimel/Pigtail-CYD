# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project status: porting to the Cheap Yellow Display (CYD)

This repo is **Pigtail-CYD**, a fork that retargets Pigtail from the M5Stack Cardputer to the
**Cheap Yellow Display "ESP32-2432S028R"** (original ESP32-WROOM, 320×240 SPI TFT, XPT2046 resistive
touch, no PSRAM). The port is a clean CYD-only fork (M5Cardputer dependency removed), touch-driven
(no keyboard), with GPS dropped. Until the phases below land, much of `src/` is still the original
Cardputer code. Key planning docs:
- `PLAN.md` — phased build checklist (current source of truth for porting work)
- `docs/PRD.md` — product requirements & Cardputer→CYD parity matrix
- `docs/devices/CYD-ESP32-2432S028R.md` — target hardware reference (pinout, quirks, RAM budget)
- `docs/devices/M5Cardputer.md` — original device reference
- `src/BoardConfig.h` — single source of truth for CYD pins/dimensions (keep hardware constants here)

## Project

Pigtail is firmware (originally for the **M5Stack Cardputer**, ESP32-S3) that passively observes nearby Wi-Fi and BLE activity, tracks how long devices linger, scores them, and renders them on a small display using icons, colors, and retro avatars. It also classifies BLE trackers (AirTag/SmartTag/Tile), smart glasses (Meta Ray-Ban, Snap, EssilorLuxottica), and Flock Safety surveillance hardware. See `README.md` for the user-facing feature list and controls.

## Build / Flash

PlatformIO project, single env `m5stack-stamps3` (board `m5stack-stamps3`, ESP32-S3, 8MB flash, custom `partitions.csv`).

```bash
pio run -e m5stack-stamps3            # build
pio run -e m5stack-stamps3 -t upload  # build + flash over USB
pio device monitor -b 115200          # serial console (USB CDC, 115200)
```

There is no test suite — CI (`.github/workflows/pio-cardputer.yml`) only builds, merges a bootable binary with `esptool merge_bin`, and attaches it to a GitHub release. CI extracts the release version from the `#define VERSION "x.y.z"` line in `src/main.cpp`, so **bump `VERSION` in `src/main.cpp` when cutting a release** — the git tag must match.

Libraries (see `platformio.ini`): M5Cardputer, NimBLE-Arduino (v2.x), TinyGPSPlus, ArduinoJson v7. Note the build uses `-std=gnu++2a` and `-Wl,-zmuldefs` (the latter is required to override `ieee80211_raw_frame_sanity_check` for promiscuous Wi-Fi).

## Architecture

The whole app is `setup()`/`loop()` in `src/main.cpp` driving two singletons: `g_tracker` (`DeviceTracker`) for sensing and `g_ui` (`UIGrid`) for rendering. `loop()` polls GPS + keyboard and calls `g_ui.update()` on a ~30 Hz frame budget; all scanning happens off-thread.

**Producer/consumer split (the key design).** `DeviceTracker` (`src/DeviceTracker.cpp`, ~2100 lines, most of the logic lives here) runs sensing in FreeRTOS tasks and ISR-context callbacks that push lightweight `Observation` structs onto a queue (`g_obs_q`). A `processing_task` drains the queue and mutates the shared state arrays. Sources:
- **Wi-Fi**: promiscuous-mode RX callback (`esp_wifi_set_promiscuous_rx_cb`) + a channel-hop task.
- **BLE**: NimBLE scan with a `ScanCB : NimBLEScanCallbacks` whose `onResult` enqueues advertisements.

Shared state is held in file-static arrays in `DeviceTracker.cpp`, guarded by a `portMUX_TYPE g_lock` spinlock — **take the lock around any access to these from a different context**:
- `g_tracks[MAX_TRACKS=256]` — Wi-Fi clients + BLE devices (`Track` in `Track.h`).
- `g_anchors[MAX_ANCHORS=128]` — Wi-Fi APs (`Anchor`).

The UI never touches these arrays directly. `UIGrid` calls `buildSnapshot()` to copy a sorted array of `EntityView` (the UI-facing, lock-free view struct) into its own buffer each frame.

**Scoring & segmentation.** Time is bucketed into windows (`WINDOW_SEC=10`, `ENV_WINDOW_SEC=30`). A device seen consistently across windows scores higher. "Segments" model the user moving through space: segment id advances via GPS displacement (~50m) when a fix is available, otherwise via AP-fingerprint change (`EnvFingerprint`). Coverage = a track's `env_hits` / `move_segments` feeds the suspicion score. `main.cpp` passes a `stationary_ratio` into the UI based on how recently the segment last advanced.

**Classification** is split into stateless inspectors that run on each advertisement and write results back onto the `Track`:
- `BleTracker` (`BleTracker.cpp`) — find-my trackers, via Apple mfg data, Google/Samsung service UUIDs, and name heuristics.
- `BleGlasses` (`BleGlasses.cpp`) — smart glasses by company ID + name.
- `BleFlock` (`BleFlock.cpp`) — Flock hardware; priority order is mfg ID `0x09C8` (XUNTONG) → Raven service UUIDs `0x3100`–`0x3500` → name → OUI `B4:1E:52`.

These enums + the `Track`/`Anchor`/`EntityView` structs all live in `src/Track.h` — the shared data-model header.

**Vendor lookup** comes from `src/MacPrefixes.h` (8500+ lines, generated — do not hand-edit). Regenerate it with `scripts/generate_mac_file.py`, which parses an nmap MAC-prefix dump into a binary-searchable `GetVendor(mac)` table and a `Vendor` enum.

**Persistence.** Watchlist/ignorelist are JSON on **SPIFFS** (`/pt_watchlist.json`, `/pt_ignorelist.json`) so tags survive reboots; KML export (`pt_watchlist.kml`, written via `k`) goes to the **SD card** and requires `_sdAvailable`. Note the watchlist file has a version field — MAC byte order changed between formats (`readWatchlist` handles the legacy NimBLE little-endian layout), so preserve that compatibility when touching serialization.

## UI rendering

`UIGrid` (`src/UIGrid.cpp`) renders everything into one off-screen `LGFX_Sprite` (an `Indexed4bppImage`, 4bpp/16-color) then pushes it, to avoid flicker. Two screens (`Grid` 7x4 tiles, and `Detail`) and three grid icon modes. Selection is "locked" to a device id/kind across list reshuffles (`syncSelectionToId`) so the cursor doesn't jump when the sorted snapshot changes underneath it.

Visual assets are compiled-in C arrays: vendor icons are 1-bit 16x16 (`Icons.h`, rendered by `Icon`/`FontRenderer`), the splash logo is 4bpp (`Logo.h`), palettes are in `Colors.h`. `RetroAvatar.cpp` deterministically generates a unique avatar per device, and `MarkovNameGenerator` + `Names.h` generate a stable human-readable name — both seeded from the MAC via `DeterministicRng` so a device always looks/names the same.
