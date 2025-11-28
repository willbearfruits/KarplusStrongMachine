# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

### Digital Kalimba
A 7-button polyphonic synthesizer using Karplus-Strong synthesis with 5 selectable musical scales.
- **File:** `DigitalKalimba.cpp`
- **Mode:** Button-triggered polyphonic playback
- **Tuning:** 5 Scales (Pentatonic, Dorian, Chromatic, Kalimba, Just)
- **Performance:** 12-15% CPU, ~115KB binary
- **Features:** 7 simultaneous voices, stereo reverb, octave shift, OLED display

**Platform:** Electrosmith Daisy Seed (STM32H750, ARM Cortex-M7)
**Language:** C++ with libDaisy and DaisySP libraries

## Build Commands

```bash
# Build
./build.sh
# or
make -j4

# Upload (DFU)
./upload.sh
# or
make program-dfu
```

**Bootloader mode:** Hold BOOT button, press RESET, release BOOT (LED pulses slowly)

## Project Structure

### Source Files
- **DigitalKalimba.cpp** - Main source code (7-button polyphonic synth)

### Build Files
- **Makefile** - Build configuration
- **build/** - Compiled binaries
- **build.sh** - Simple build wrapper
- **upload.sh** - Simple upload wrapper

### Documentation
- **CLAUDE.md** - This file (project documentation)
- **KALIMBA_WIRING.md** - Complete wiring guide
- **README.md** - General project information

### Web Flasher
- **web-flasher/** - Browser-based firmware flasher using WebUSB
  - **URL:** https://willbearfruits.github.io/KarplusStrongMachine/web-flasher/

## Architecture

### Audio Processing Chain
```
Buttons (D1-D7)
    ↓
String class (Karplus-Strong) × 7 voices
    ├─ Frequency Control (Scale + Octave)
    ├─ Lowpass filter (Brightness A0)
    └─ Feedback loop (Decay A1)
    ↓
Stereo ReverbSc (Mix A4, Time A5)
    ↓
DC Blocker
    ↓
Stereo Output (Pins 19/20)
```

### Hardware Mapping

**Buttons (Active Low, Pull-up):**
- D1 (Pin 2) to D7 (Pin 8) → Buttons 1-7

**Potentiometers (A0-A5):**
- A0: Brightness (Tone)
- A1: Decay (Sustain)
- A2: Octave Shift (-2 to +2)
- A3: Scale Select (5 zones)
- A4: Reverb Mix
- A5: Reverb Time

**OLED Display (I2C):**
- SCL → D11 (Pin 12)
- SDA → D12 (Pin 13)

## Code Modification Guidelines

### Audio Callback
- Keep processing lightweight
- Use `fclamp()` for control parameters
- Process controls once per block, not per sample

### Parameters
- `OCTAVE_RATIOS`: Lookup table for octave shifting
- `scale_frequencies`: 2D array [Scale][String]

## Common Issues
- **Web Flasher Timeout:** Ensure using v1.0.7+ (1024 byte transfer size)
- **No Audio:** Check amp/headphone connection to Pins 19/20
- **Wrong Scale:** Check A3 knob position
