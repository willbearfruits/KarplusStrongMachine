# Digital Kalimba - Wiring Guide

## Overview
This is a 7-button digital kalimba using Karplus-Strong synthesis on the Daisy Seed. It uses G Major Pentatonic tuning (traditional kalimba scale).

## Button Connections

### Basic Wiring (Momentary Push Buttons)

Each button connects between a Daisy GPIO pin and GND. The firmware uses internal pull-up resistors, so the buttons are active-low (pressed = LOW signal).

```
Button Wiring (each button):

    Daisy GPIO Pin ────┐
                       │
                    [Button]
                       │
                      GND
```

### Pin Assignments

| Button # | Note | Frequency | GPIO Pin | Physical Pin | Wiring |
|----------|------|-----------|----------|--------------|--------|
| 1 | G4 (Center) | 392 Hz | D1 | Pin 2 | D1 → Button → GND |
| 2 | A3 (Left 1) | 220 Hz | D2 | Pin 3 | D2 → Button → GND |
| 3 | B4 (Right 1) | 494 Hz | D3 | Pin 4 | D3 → Button → GND |
| 4 | D4 (Left 2) | 294 Hz | D4 | Pin 5 | D4 → Button → GND |
| 5 | E4 (Right 2) | 330 Hz | D5 | Pin 6 | D5 → Button → GND |
| 6 | G3 (Left 3) | 196 Hz | D6 | Pin 7 | D6 → Button → GND |
| 7 | A4 (Right 3) | 440 Hz | D7 | Pin 8 | D7 → Button → GND |

### Traditional Kalimba Layout

Physical button arrangement (player's view):

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

## Potentiometer Controls

Connect 6 potentiometers (10kΩ linear recommended) to analog inputs:

### Wiring (each pot):
```
    3.3V ─── Pin 3 (CW)

    ADC ───── Pin 2 (Wiper)

    GND ──── Pin 1 (CCW)
```

**⚠️ IMPORTANT:** Use 3.3V, NOT 5V! Daisy Seed analog inputs are 3.3V max.

### Control Assignments

| Pot | ADC Pin | Physical Pin | Function | Range |
|-----|---------|--------------|----------|-------|
| 1 | A0 | Pin 15 | Global Brightness | 0.5 - 1.0 (darker - brighter) |
| 2 | A1 | Pin 16 | Global Decay/Sustain | 0.5 - 1.0 (short - long) |
| 3 | A2 | Pin 17 | Octave Shift | -2 to +2 octaves (5 zones) |
| 4 | A3 | Pin 18 | Scale Select | 5 scales (Pentatonic, Dorian, Chromatic, Kalimba, Just) |
| 5 | A4 | Pin 19 | Reverb Mix | 0 - 100% (dry - wet) |
| 6 | A5 | Pin 20 | Reverb Time | 0.6 - 0.999 (decay/feedback) |

## OLED Display (Optional)

0.96" SSD1306 I2C Display (128x64)

### Wiring:
```
OLED VCC → Daisy 3.3V
OLED GND → Daisy GND
OLED SCL → D11 (Pin 12)
OLED SDA → D12 (Pin 13)
```

**I2C Address:** 0x3C (default) or 0x3D

## Audio Output

Connect the Daisy Seed audio output to your amplifier/speakers:

```
Audio Out Left  (Pin 19) → Amplifier Left Input
Audio Out Right (Pin 20) → Amplifier Right Input
Audio Ground    (Pin 40) → Amplifier Ground
```

The kalimba generates mono audio (same on both channels).

## Power

Power the Daisy Seed via:
- **USB:** Micro USB (5V from computer or USB power adapter)
- **VIN:** Pin 39 (9-12V DC, center-positive)

**Current draw:** ~300mA @ 5V typical

## Complete Wiring Diagram

```
                        ┌─────────────────────────┐
                        │    Daisy Seed Board     │
                        │                         │
    Buttons:            │                         │
    [1] ────────────────│ D1 (2)                  │
    [2] ────────────────│ D2 (3)                  │
    [3] ────────────────│ D3 (4)                  │
    [4] ────────────────│ D4 (5)                  │
    [5] ────────────────│ D5 (6)                  │
    [6] ────────────────│ D6 (7)                  │
    [7] ────────────────│ D7 (8)                  │
         │              │                         │
         └─── All to GND (40)                     │
                        │                         │
    Pots (wipers):      │                         │
    Pot 1 (Bright) ─────│ A0 (15)                 │
    Pot 2 (Decay) ──────│ A1 (16)                 │
    Pot 3 (Octave) ─────│ A2 (17)                 │
    Pot 4 (Scale) ──────│ A3 (18)                 │
    Pot 5 (Rev Mix) ────│ A4 (19)                 │
    Pot 6 (Rev Time) ───│ A5 (20)                 │
                        │                         │
    OLED:               │                         │
    SCL ────────────────│ D11 (12)                │
    SDA ────────────────│ D12 (13)                │
    VCC ────────────────│ 3.3V (21)               │
    GND ────────────────│ GND (40)                │
                        │                         │
    Audio Out:          │                         │
    Left ───────────────│ Audio Out 1 (19)        │
    Right ──────────────│ Audio Out 2 (20)        │
    Ground ─────────────│ GND (40)                │
                        │                         │
    Power:              │                         │
    USB ────────────────│ Micro USB port          │
                        │                         │
                        └─────────────────────────┘
```

## Recommended Components

### Buttons
- **Type:** Momentary tactile switches (NO - Normally Open)
- **Options:**
  - Arcade buttons (30mm) for large, satisfying presses
  - Cherry MX mechanical switches for clicky feedback
  - Standard momentary push buttons (12mm)
- **Rating:** 50mA minimum (Daisy GPIO can sink 25mA)

### Potentiometers
- **Value:** 10kΩ linear (B10K)
- **Type:** Panel mount, 6mm shaft
- **Quantity:** 6

### OLED Display
- **Size:** 0.96" diagonal
- **Resolution:** 128x64
- **Interface:** I2C (4-pin)
- **Driver:** SSD1306
- **Color:** White, Blue, or Yellow/Blue

### Enclosure Ideas
- 3D-printed case with button layout
- Wooden box for authentic kalimba aesthetic
- Laser-cut acrylic panel
- Desktop breadboard prototype

## Testing Procedure

### 1. Button Test
After wiring, test each button individually:
```bash
./build-and-upload-kalimba.sh
```

Press each button - you should:
- Hear the corresponding note
- See the LED flash
- See the note name on OLED display

### 2. Tuning Verification
Use a guitar tuner app to verify frequencies:
- Button 1 should show G4
- Button 2 should show A3
- Button 3 should show B4
- etc.

### 3. Polyphony Test
Press multiple buttons simultaneously - all should sound together without glitches.

### 4. Control Test
Turn each potentiometer and verify:
- A0: Brightness changes (duller/brighter tone)
- A1: Decay changes (shorter/longer sustain)
- A2: Reverb amount changes
- A3: Reverb character changes
- A4/A5: Optional vibrato (may be subtle)

## Troubleshooting

### Button not triggering:
- Check button wiring (GPIO pin to GND)
- Verify button is momentary (not latching)
- Check for cold solder joints
- Test button with multimeter (should show continuity when pressed)

### Wrong note plays:
- Verify GPIO pin assignment in code
- Check button is connected to correct pin
- Re-upload firmware

### No audio output:
- Check audio cable connections
- Verify amplifier is on and volume up
- Test with original KarplusStrongMachine firmware
- Check codec initialization (1-second delay present)

### Display not working:
- Check I2C wiring (SCL/SDA not swapped)
- Verify I2C address (0x3C or 0x3D)
- Check 3.3V power to display
- Firmware works without display (non-critical)

### Pots not responding:
- Verify 3.3V and GND connections
- Check wiper connection to ADC pin
- Use multimeter to verify pot resistance changes
- Ensure ADC.Start() is called in code

## Safety Notes

⚠️ **Never apply 5V to analog inputs** - Use 3.3V only
⚠️ **Don't hot-swap** - Power off before connecting/disconnecting Daisy
⚠️ **Check polarity** - VIN is center-positive 9-12V DC
⚠️ **No shorting** - Avoid shorting 3.3V to GND

## Performance Specs

- **CPU Usage:** ~12-15% (88% available for future expansion)
- **Latency:** ~0.08ms (4-sample blocks @ 48kHz)
- **Polyphony:** 7 voices (all can play simultaneously)
- **Sustain Range:** 2-10 seconds (frequency-dependent)
- **Flash Usage:** ~95KB / 128KB (33KB free)
- **RAM Usage:** ~30KB / 512KB (482KB free)

## Future Expansion Ideas

With 88% CPU headroom, you can add:
- Delay effect
- Chorus/flanger
- More complex LFO modulation
- MIDI input/output
- Multiple tuning presets (selectable via button combo)
- Recording/looping
- More voices (expand to 8-10 notes)

---

## Quick Reference: Pin Summary

**Buttons (all to GND):** D1 (Pin 2), D2 (3), D3 (4), D4 (5), D5 (6), D6 (7), D7 (8)
**Pots (wipers):** A0(Bright), A1(Decay), A2(Octave), A3(Scale), A4(Mix), A5(Time)
**OLED I2C:** D11 (SCL), D12 (SDA)
**Power:** USB or VIN (Pin 39)
**Audio:** Pins 19 (L), 20 (R), 40 (GND)

---

**For more details, see:**
- `DigitalKalimba.cpp` - Main source code
- `CLAUDE.md` - Project documentation
- Daisy Seed pinout diagram: `Daisy_Seed_pinout_square.png`
