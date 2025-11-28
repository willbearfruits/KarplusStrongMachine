# Digital Kalimba for Daisy Seed

A 7-button polyphonic synthesizer using Karplus-Strong physical modeling synthesis with 5 selectable musical scales.

## Features

- **Full Polyphony:** All 7 notes can play simultaneously
- **5 Musical Scales:** Pentatonic Major, Dorian, Chromatic, Kalimba, Just Intonation
- **Octave Shift:** -2 to +2 octaves (5 octave range)
- **Frequency-Dependent Decay:** Low notes sustain longer, high notes decay faster
- **Stereo Reverb:** High-quality ReverbSc for spatial depth
- **Real-Time Control:** Brightness, Decay, Scale Select, Reverb Mix/Time
- **OLED Display:** Shows current scale, octave, and active notes
- **Low Latency:** ~0.08ms

## Quick Start (Easiest Way)

**Flash directly from your browser! No software installation required.**

1.  **Connect** your Daisy Seed to your computer via USB.
2.  **Visit the Web Flasher:**  
    👉 **[https://willbearfruits.github.io/KarplusStrongMachine/web-flasher/](https://willbearfruits.github.io/KarplusStrongMachine/web-flasher/)**
3.  **Put Daisy in Bootloader Mode:**
    *   Hold **BOOT** button.
    *   Press **RESET** button.
    *   Release **BOOT** button.
4.  Click **"Connect to Daisy Seed"** and select your device.
5.  Click **"Flash Firmware"**.
6.  Press **RESET** to play!

---

## Alternative: Build from Source (CLI)

### 1. Build and Upload

```bash
cd ~/Documents/KarplusStrongMachine
make program-dfu
```

## Hardware Setup

### Minimum Required Wiring

**7 Buttons (to GND):**
- D1 (Pin 2) - G4 (Center note)
- D2 (Pin 3) - A3
- D3 (Pin 4) - B4
- D4 (Pin 5) - D4
- D5 (Pin 6) - E4
- D6 (Pin 7) - G3 (Lowest note)
- D7 (Pin 8) - A4

Each button connects GPIO pin to GND (momentary push buttons, normally open).

**Audio Output:**
- Pin 19 (Left) → Amplifier/Headphones
- Pin 20 (Right) → Amplifier/Headphones
- Pin 40 (GND) → Common ground

### Optional Controls

**6 Potentiometers (10kΩ, wiper to ADC pin):**
- A0 (Pin 15) - Global Brightness
- A1 (Pin 16) - Global Decay/Sustain
- A2 (Pin 17) - Octave Shift
- A3 (Pin 18) - Scale Select
- A4 (Pin 19) - Reverb Mix
- A5 (Pin 20) - Reverb Time

**OLED Display (I2C, 0.96" SSD1306):**
- D11 (Pin 12) - SCL
- D12 (Pin 13) - SDA
- 3.3V - VCC
- GND - GND

For complete wiring instructions, see **KALIMBA_WIRING.md**

## Button Layout

Traditional kalimba center-out alternating pattern:

```
      Left Side          Center         Right Side

        [6] G3
             [4] D4
                  [2] A3    [1] G4
                                    [3] B4
                                [5] E4
                                    [7] A4

   Playing order (low to high):
   6 (G3) → 2 (A3) → 4 (D4) → 5 (E4) → 1 (G4) → 7 (A4) → 3 (B4)
```

**Tuning:** G Major Pentatonic
**Scale degrees:** G A B D E G A
**Range:** G3 (196 Hz) to B4 (493.88 Hz)

## Note Characteristics

| Button | Note | Frequency | Sustain | Character |
|--------|------|-----------|---------|-----------|
| 1 | G4 | 392 Hz | Medium-Long | Balanced |
| 2 | A3 | 220 Hz | Very Long | Warm, deep |
| 3 | B4 | 494 Hz | Short | Bright, clear |
| 4 | D4 | 294 Hz | Long | Rich, full |
| 5 | E4 | 330 Hz | Medium | Sweet |
| 6 | G3 | 196 Hz | Longest | Deep, resonant |
| 7 | A4 | 440 Hz | Medium | Bright |

## Controls (Potentiometers)

| Control | Function | Effect |
|---------|----------|--------|
| **A0 - Brightness** | Tone color | CCW: Warmer, darker / CW: Brighter, metallic |
| **A1 - Decay** | Sustain time | CCW: Short plucks / CW: Long sustain |
| **A2 - Octave Shift** | Pitch range | Center: Normal. Left: -1/-2 oct. Right: +1/+2 oct |
| **A3 - Scale Select** | Musical Scale | 5 regions: Pentatonic -> Dorian -> Chromatic -> Kalimba -> Just Intonation |
| **A4 - Reverb Mix** | Dry/wet mix | CCW: Dry / CW: Full reverb |
| **A5 - Reverb Time** | Room size | CCW: Small room / CW: Large hall |

## Tips for Playing

### Basic Technique
- Press buttons firmly for clean attack
- Multiple buttons can be pressed simultaneously for chords
- Experiment with rhythmic patterns

### Musical Ideas
- **Arpeggio:** Play 6-2-4-5-1-7-3 slowly for ascending melody
- **Chord:** Press 6+4+1 together for G major triad
- **Drone:** Hold button 6 (G3) while playing melody on other buttons
- **Rhythm:** Use buttons 1 and 3 alternating for simple rhythm

### Sound Shaping
- **Plucked sound:** Brightness high (A0 CW), Decay medium (A1 center)
- **Bell-like:** Brightness very high (A0 full CW), Reverb high (A4/A5 CW)
- **Warm pad:** Brightness low (A0 CCW), Decay high (A1 CW), Reverb high (A4 CW)
- **Octave Jump:** Turn A2 left/right to shift range down/up
- **Scale Change:** Turn A3 to explore different moods (Dorian, Chromatic, etc.)

## Technical Specifications

**Platform:** Electrosmith Daisy Seed (STM32H750, ARM Cortex-M7 @ 480MHz)
**DSP Algorithm:** Karplus-Strong string synthesis with DC blocking
**Audio Quality:** 48kHz sample rate, floating-point processing
**Polyphony:** 7 voices (fully independent strings)
**Effects:** ReverbSc, optional LFO modulation (vibrato/tremolo)
**Latency:** ~0.08ms (4-sample buffer)
**CPU Usage:** 12-15%
**Memory:** ~95KB Flash, ~30KB RAM

## Customization

The tuning and parameters can be easily modified in `DigitalKalimba.cpp`:

### Change Tuning
Edit the `base_frequencies[]` array:
```cpp
// Example: C Major Pentatonic
const float base_frequencies[NUM_STRINGS] = {
    261.63f,  // C4
    146.83f,  // D3
    329.63f,  // E4
    196.00f,  // G3
    440.00f,  // A4
    130.81f,  // C3
    293.66f   // D4
};
```

### Adjust Sustain Times
Edit the `base_damping[]` array (0.5 = short, 0.99 = very long):
```cpp
const float base_damping[NUM_STRINGS] = {
    0.95f, 0.95f, 0.95f, 0.95f, 0.95f, 0.95f, 0.95f
};
```

### Change Button Pins
Edit the `button_pins[]` array to use different GPIO pins.

## Troubleshooting

**No sound:**
- Check audio cable connections
- Verify buttons are wired correctly (GPIO to GND)
- Ensure Daisy is powered and reset after upload

**Wrong notes:**
- Verify button connections match pin assignments
- Check for swapped wires
- Re-upload firmware

**Buttons not responding:**
- Test button continuity with multimeter
- Check for cold solder joints
- Verify GND connection

**Display not working:**
- Non-critical - kalimba works without display
- Check I2C wiring (SCL/SDA not swapped)
- Verify 3.3V power to display

**Pots not responding:**
- Optional feature - kalimba works without pots
- Check 3.3V and GND connections
- Verify wiper connection to ADC pin

## Future Enhancements

With 85% CPU remaining, you can add:
- **Delay effect** for rhythmic echoes
- **Chorus** for lush, shimmering texture
- **Multiple tuning presets** (button combo to switch)
- **Recording/looping**
- **Velocity sensitivity** (FSR sensors)
- **MIDI output** to control other synths
- **Sympathetic resonance** (notes trigger related harmonics)
- **Expand to 8-12 notes**

## Files

- **DigitalKalimba.cpp** - Main source code
- **Makefile** - Build configuration
- **web-flasher/** - WebUSB firmware uploader
- **KALIMBA_WIRING.md** - Complete wiring guide
- **build.sh / upload.sh** - Convenience scripts


## Credits

Based on the KarplusStrongMachine project by glitches.
Uses libDaisy and DaisySP libraries by Electrosmith.
Karplus-Strong algorithm ported from Mutable Instruments Rings.

## License

MIT License (same as libDaisy/DaisySP)

---

**Enjoy your Digital Kalimba!** 🎵

For questions or issues, check KALIMBA_WIRING.md or CLAUDE.md for detailed documentation.
