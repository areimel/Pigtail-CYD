// BoardConfig.h
//
// Single source of truth for the target board's pin assignments and dimensions.
// Target: Cheap Yellow Display "ESP32-2432S028R" (2.8" SPI TFT, original ESP32-WROOM).
//
// Keeping every hardware-specific constant here (rather than scattered through
// main.cpp / Display.cpp / DeviceTracker.cpp) keeps the port DRY and makes it
// straightforward to re-target another CYD variant later: change this file only.
//
// Sources: witnessmenow/ESP32-Cheap-Yellow-Display (cyd.md), Random Nerd Tutorials
// CYD pinout, mischianti ESP32-2432S028 pinout. See docs/devices/CYD-ESP32-2432S028R.md.

#pragma once

#include <cstdint>

namespace board {

// ---------------------------------------------------------------------------
// Display panel (SPI TFT) — ESP32-2432S028R
//   The board ships with EITHER an ILI9341 or an ST7789 controller depending on
//   batch. Select with -DCYD_PANEL_ST7789 (default is ILI9341). See Display.cpp.
// ---------------------------------------------------------------------------
namespace tft {
constexpr int SCLK = 14;
constexpr int MOSI = 13;
constexpr int MISO = 12;
constexpr int CS   = 15;
constexpr int DC   = 2;
constexpr int RST  = -1;  // Not wired to a GPIO on this board (tied to EN/reset).
                          // NOTE: GPIO4 is the RGB LED red channel, NOT TFT reset,
                          // despite some community pinouts mislabeling it.
constexpr int BL   = 21;  // Backlight (PWM-capable for brightness).
constexpr int WIDTH  = 320; // landscape (native panel is 240x320 portrait)
constexpr int HEIGHT = 240;
constexpr uint32_t SPI_FREQ_HZ = 40000000; // 40 MHz write clock
}  // namespace tft

// ---------------------------------------------------------------------------
// Touch (XPT2046 resistive) — on its OWN SPI pins, separate from the TFT bus.
//   Configured as a touch panel inside the LovyanGFX device (Display.cpp), so it
//   shares the panel's software/bus handling. Requires one-time calibration.
// ---------------------------------------------------------------------------
namespace touch {
constexpr int CLK  = 25;
constexpr int MOSI = 32;
constexpr int MISO = 39;
constexpr int CS   = 33;
constexpr int IRQ  = 36;  // input-only pin
constexpr uint32_t SPI_FREQ_HZ = 2500000; // 2.5 MHz — XPT2046 is slow
}  // namespace touch

// ---------------------------------------------------------------------------
// microSD card — VSPI bus (shared-bus considerations with touch; see docs).
// ---------------------------------------------------------------------------
namespace sd {
constexpr int CS   = 5;
constexpr int MOSI = 23;
constexpr int MISO = 19;
constexpr int SCK  = 18;
constexpr uint32_t FREQ_HZ = 20000000; // 20 MHz
}  // namespace sd

// ---------------------------------------------------------------------------
// Onboard extras.
// ---------------------------------------------------------------------------
namespace led {
// Common-anode RGB LED, ACTIVE LOW (drive pin LOW to light the channel).
constexpr int R = 4;   // shared concern: GPIO4 — see tft::RST note above
constexpr int G = 16;
constexpr int B = 17;
}  // namespace led

constexpr int LDR     = 34;  // ambient light sensor (ADC1, input-only)
constexpr int SPEAKER = 26;  // small speaker via onboard amp (drive with LEDC tone)
constexpr int BOOT_BTN = 0;  // BOOT button (also strapping pin)

// ---------------------------------------------------------------------------
// Free / exposed GPIO on the P3 / CN1 / P1 headers (for reference; very limited).
//   GPIO 22, 27 (general purpose), GPIO 35 (input-only, ADC). GPIO 21 doubles as
//   backlight. A future optional external GPS UART could live on 22/27.
// ---------------------------------------------------------------------------

}  // namespace board
