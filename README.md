# KarplusStrongMachine – Digital Kalimba on Daisy Seed

A playable **Karplus–Strong “digital Kalimba”** built on the Daisy Seed platform.  
Designed as both an **instrument** and a **teaching tool** for exploring physical modelling, microcontrollers, and sound design.

**Status:** Workshop-ready; evolving with student feedback.

## Support

Most of my work stays free and open-source.
Support early access, live sessions, and workshop-time:
https://www.patreon.com/Seriousshit

---

## 🎯 What this project is

- A complete **hardware + firmware** project:
  - Daisy Seed-based audio core
  - Simple controls for tuning, decay, damping, etc.
  - Audio out designed for pedal/synth workflows
- A **workshop-ready** instrument:
  - **[Web Flasher](https://willbearfruits.github.io/KarplusStrongMachine/web-flasher/)** so students can program it from a browser (no toolchain needed!)
  - Clear wiring diagrams and parts list
  - Documentation written for people who like to experiment and break things

---

## 🧱 Features

- **Karplus–Strong synthesis** (plucked string / Kalimba vibe) with 7-voice polyphony
- **5 Selectable Scales** (Pentatonic, Dorian, Chromatic, Kalimba, Just Intonation)
- **Stereo Reverb** (ReverbSc) for spatial depth
- **Octave Shift** (-2 to +2 range)
- **Low Latency** (~0.08ms) for responsive playability
- **Parameters exposed** as simple potentiometers, making it easy to teach DSP concepts like "damping" and "feedback" physically.

---

## 🔌 Hardware Overview

- **Core:** Electrosmith Daisy Seed (STM32H750)
- **Controls:**  
  - 7 Push Buttons (Notes D1-D7)
  - 6 Potentiometers (Tone, Decay, Octave, Scale, Reverb Mix, Reverb Time)
  - 1 OLED Display (Optional, I2C)
- **I/O:**  
  - Stereo Audio Out (Pins 19/20)
  - USB for flashing

See **[KALIMBA_WIRING.md](KALIMBA_WIRING.md)** for complete wiring details and pinouts.

---

## 🚀 Quickstart

### 1. The Easy Way (WebUSB)
No coding required. Perfect for workshops.
1. Connect Daisy Seed via USB.
2. Put in bootloader mode (Hold BOOT, press RESET).
3. Go to the **[Web Flasher](https://willbearfruits.github.io/KarplusStrongMachine/web-flasher/)**.
4. Click "Connect" -> "Flash".

### 2. The Developer Way (CLI)
For modifying the code and DSP.

```bash
git clone https://github.com/willbearfruits/KarplusStrongMachine.git
cd KarplusStrongMachine

# Build
make -j4

# Flash
make program-dfu
```

---

## 🎓 Teaching & Workshop Use

I use this project in workshops on:
- **Synths and Microcontrollers**
- **Physical Modelling Synthesis**
- **Building Complete Instruments** (DSP + Hardware + Enclosure)

It is intentionally:
- **Minimal enough** for beginners to follow the signal path.
- **Open enough** for advanced students to fork (e.g., "change the scale," "add a delay effect," "make it a drone machine").

If you use it in a class, feel free to open an issue or PR with notes!

---

## 🧪 Technical Details

- **Platform:** Daisy Seed (ARM Cortex-M7 @ 480MHz)
- **Audio:** 48kHz, 24-bit, 4-sample block size
- **DSP:** Custom `String` class (ported from Mutable Instruments Rings)
- **CPU Usage:** ~15% (plenty of room for more effects)

## License

MIT. Break it, bend it, rebuild it.
