# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains two Karplus-Strong synthesis projects for the Daisy Seed:

### 1. Karplus-Strong Plucked String Machine
A physical modeling synthesizer featuring the Karplus-Strong algorithm with triple LFO modulation and long decay times.
- **File:** `KarplusStrongMachine_Final.cpp`
- **Mode:** Auto-trigger with speed control
- **Performance:** 5-8% CPU, 90KB binary

### 2. Digital Kalimba (NEW!)
A 7-button polyphonic kalimba instrument using Karplus-Strong synthesis with traditional G Major Pentatonic tuning.
- **File:** `DigitalKalimba.cpp`
- **Mode:** Button-triggered polyphonic playback
- **Tuning:** G Major Pentatonic (G3-B4 range)
- **Performance:** 12-15% CPU, ~95KB binary
- **Features:** 7 simultaneous voices, reverb, LFO modulation, OLED display

**Platform:** Electrosmith Daisy Seed (STM32H750, ARM Cortex-M7)
**Language:** C++ with libDaisy and DaisySP libraries

## Build Commands

### Karplus-Strong Machine (Original)
```bash
# Build and upload
./build-and-upload.sh

# Build only
./build.sh
# or
make clean && make -j4

# Upload only
./upload.sh
# or
make program-dfu
```

### Digital Kalimba (New)
```bash
# Build and upload
./build-and-upload-kalimba.sh

# Build only
./build-kalimba.sh

# Upload only
./upload-kalimba.sh
```

**Bootloader mode:** Hold BOOT button, press RESET, release BOOT (LED pulses slowly)

## Project Structure

### Source Files

**Karplus-Strong Machine:**
- **KarplusStrongMachine_Final.cpp** - Current production version (auto-trigger mode with speed control)
- **KarplusStrongMachine.cpp** - Original version (manual trigger via A3)
- **KarplusStrongMachine_DualMode.cpp** - Experimental dual-mode version
- **KarplusStrongMachine_AutoTrigger.cpp** - Auto-trigger variant

**Digital Kalimba:**
- **DigitalKalimba.cpp** - 7-button polyphonic kalimba instrument

### Build Files

- **Makefile** - Points to `KarplusStrongMachine_Final.cpp`
- **Makefile.kalimba** - Points to `DigitalKalimba.cpp`
- **build/** - Compiled binaries (.bin, .elf, .map)
- **build.sh** / **build-kalimba.sh** - Build scripts with dependency checks
- **upload.sh** / **upload-kalimba.sh** - DFU upload scripts
- **build-and-upload.sh** / **build-and-upload-kalimba.sh** - Combined workflows

### Documentation

- **CLAUDE.md** - This file (project documentation)
- **KALIMBA_WIRING.md** - Complete wiring guide for Digital Kalimba
- **README.md** - General project information
- **Daisy_Seed_pinout_square.png** - Pinout reference
- **Electrosmith Daisy Seed_ Technical Overview.pdf** - Hardware specs

### Web Flasher

- **web-flasher/** - Browser-based firmware flasher using WebUSB
  - No installation required - flash directly from browser
  - Works with Chrome/Edge/Opera (WebUSB support required)
  - See web-flasher/README.md for details

### Diagnostics (Development Files)

- **diagnostics/** - Diagnostic tools and development notes
  - Test programs for debugging audio codec, OLED, etc.
  - Development versions and troubleshooting documentation
  - Not needed for normal use

## Dependencies

Located at `../../DaisyExamples/`:

- **libDaisy** - Hardware abstraction layer for Daisy platform
- **DaisySP** - DSP library with synthesis modules

Both must be present at `~/DaisyExamples/libDaisy` and `~/DaisyExamples/DaisySP`.

## Architecture

### Audio Processing Chain

```
Trigger System (auto or manual)
    ↓
String class (Karplus-Strong synthesis)
    ├─ Delay line (pitch-dependent buffer)
    ├─ Lowpass filter (brightness)
    └─ Feedback loop (decay/damping)
    ↓
Triple LFO Modulation
    ├─ LFO 1: Vibrato (sine, pitch ±2%)
    ├─ LFO 2: Tremolo (triangle, amplitude 0-50%)
    └─ LFO 3: Filter Sweep (sawtooth, brightness ±30%)
    ↓
DC Blocker
    ↓
Soft Saturation (tanh)
    ↓
Mono Output → Both Channels
```

### DaisySP Classes Used

- **String** - Karplus-Strong implementation (ported from Mutable Instruments Rings)
- **Oscillator** - LFO waveform generators (sine, triangle, saw)
- **AnalogControl** - Filtered analog input processing (prevents zipper noise)
- **DcBlock** - DC offset removal

### Control Processing

- **Sample Rate:** 48 kHz
- **Block Size:** 4 samples (~0.083ms latency)
- **ADC:** 6 channels (A0-A5), DMA-driven, non-blocking
- **Control Rate:** ~1 kHz (once per audio block)
- **Exponential Scaling:** Pitch and LFO rate use `powf()` for musical response

### Hardware Mapping

**Analog Inputs (A0-A5):**
- A0 (Pin 15): Pitch (50 Hz - 2000 Hz, exponential)
- A1 (Pin 16): Decay Time (1s to 20s+, linear damping coefficient 0.0-1.0)
- A2 (Pin 17): Brightness (filter cutoff, linear)
- A3 (Pin 18): Trigger Speed (0.1s - 10s auto-trigger interval in Final version)
- A4 (Pin 19): LFO Rate (0.1 Hz - 20 Hz, exponential)
- A5 (Pin 20): LFO Depth (0-100%, linear)

**Potentiometer Wiring (each pot):**
```
Pin 1 (CCW) → GND
Pin 2 (Wiper) → A0-A5 analog input pin
Pin 3 (CW) → 3.3V (⚠️ NOT 5V!)
```

**Audio Outputs:**
- Physical Pin 18: Left channel (audio codec output)
- Physical Pin 19: Right channel (same mono signal)

**OLED Display (0.96" SSD1306, I2C, 128x64):**
- VCC → 3.3V power
- GND → GND
- SCL → Pin 12 (GPIO PB8, I2C1_SCL)
- SDA → Pin 13 (GPIO PB9, I2C1_SDA)
- I2C Address: 0x3C

**LED:** Onboard LED blinks on each string pluck trigger

## Important Technical Details

### Audio Codec Initialization

**CRITICAL:** The AK4556 audio codec requires a 1-second delay after `hw.Init()` to properly stabilize before starting audio. Without this delay, audio output may be intermittent or fail entirely.

```cpp
hw.Init();
hw.SetAudioBlockSize(4);
System::Delay(1000);  // REQUIRED for reliable audio codec initialization
```

This delay allows the codec to:
- Power up internal circuits
- Stabilize clock generation
- Initialize DAC converters

**Symptom if missing:** Audio works sometimes but not consistently after reset/power-up.

### Version Differences

The Makefile currently compiles `KarplusStrongMachine_Final.cpp`, which uses **auto-trigger mode**:
- A3 controls trigger speed (0.1s - 10s interval)
- String automatically plucks at intervals
- LED flashes on each trigger

To switch to manual trigger mode, edit Makefile to use `KarplusStrongMachine.cpp`:
- A3 becomes manual trigger (turn past 60% threshold)
- Rising edge detection with 100ms lockout
- More traditional performance control

### Memory Constraints

- **Flash:** 91,220 / 131,072 bytes (69.6%) - 40KB available
- **SRAM:** 18,764 / 524,288 bytes (3.6%) - mostly delay buffers
- **RAM_D2:** 16,704 / 294,912 bytes (5.7%) - DMA buffers

CPU has 92-95% headroom for additional effects (reverb, delay, chorus, polyphony).

### Karplus-Strong Parameters

The `daisysp::String` class provides:
- `SetFreq(float)` - Fundamental frequency
- `SetDamping(float)` - Decay time (0.0 = short, 1.0 = infinite)
- `SetBrightness(float)` - Filter cutoff (0.0 = dark, 1.0 = bright)
- `SetNonLinearity(float)` - String stiffness (set to 0.1 in this project)
- `Process(bool trigger)` - Returns audio sample, trigger=true excites string

### LFO Rates and Phases

The three LFOs run at different relative speeds to create complex modulation:
- Vibrato: 1.0× base rate (from A4)
- Tremolo: 0.7× base rate
- Filter: 0.4× base rate

All three are scaled by A5 (depth control).

## Code Modification Guidelines

### When Editing Audio Callback

- Keep processing lightweight - target <10% CPU per feature
- Use `fclamp()` to prevent parameters from going out of range
- Exponential scaling for frequency parameters: `powf(base, control_value)`
- Linear scaling for amplitude/time parameters
- Process controls once per block, not per sample

### Adding New DSP Modules

1. Declare global instance (before `AudioCallback`)
2. Initialize in `main()` with `sample_rate`
3. Process in audio callback loop
4. Map to unused analog inputs or create menu system

### Trigger System Modification

Current auto-trigger uses sample-based timer:
```cpp
trigger_timer++;
if (trigger_timer >= trigger_interval) {
    trigger = true;
    trigger_timer = 0;
}
```

For external gate/CV input, replace with GPIO edge detection.

## Testing and Debugging

### Build Verification

Successful build shows:
```
✅ BUILD SUCCESSFUL!
-rwxrwxr-x 1 ... 90K ... KarplusStrongMachine.bin
```

### Upload Verification

Successful upload shows:
```
Download [=========================] 100%
File downloaded successfully
```

After upload, press RESET button on Daisy Seed.

### Audio Testing

1. Connect audio output (pins 22/23) to headphones or speakers
2. Turn A3 to trigger (manual) or wait for auto-trigger
3. Adjust A0 (pitch) - should hear frequency change
4. Adjust A1 (decay) - should hear sustain length change
5. Adjust A5 (LFO depth) - should hear modulation effects

### Common Issues

**No sound:** Check audio wiring, trigger threshold (A3), power connection
**Glitching/dropouts:** CPU overload - reduce effects or optimize processing
**Unwanted retriggering:** Adjust trigger threshold or lockout time
**No modulation:** Turn up A5 (LFO depth), adjust A4 (LFO rate)

## Extension Ideas

### Karplus-Strong Machine
With 92% CPU remaining:
- Reverb (freeverb, plate): 10-15% CPU
- Delay/echo: 5-8% CPU
- Polyphony (4-6 voices): 20-30% CPU
- MIDI input: minimal overhead
- External CV/gate: GPIO + ADC configuration

### Digital Kalimba
With 85% CPU remaining:
- Additional tuning presets (selectable via button combo)
- Delay effect for rhythmic echoes
- Chorus for shimmering texture
- Recording/looping functionality
- Expand to 8-10 notes
- Velocity sensitivity via FSRs (force-sensitive resistors)
- MIDI output for controlling other synths
- Sympathetic resonance between related notes

See README.md "Future Enhancements" for detailed expansion plans.

## Digital Kalimba Specific Information

### Button Layout and Tuning

The Digital Kalimba uses traditional kalimba layout with center-out alternating pattern:

```
   Left Side          Center         Right Side
     [6] G3
          [4] D4
               [2] A3    [1] G4
                                [3] B4
                            [5] E4
                                [7] A4
```

**Scale:** G Major Pentatonic
**Range:** G3 (196 Hz) to B4 (493.88 Hz)
**Tuning:** Traditional kalimba tuning used by Hugh Tracey instruments

### Hardware Connections

**Buttons (active-low with pull-ups):**
- Button 1 → D15 (Pin 23) → GND - G4 (Center)
- Button 2 → D16 (Pin 24) → GND - A3 (Left 1)
- Button 3 → D17 (Pin 25) → GND - B4 (Right 1)
- Button 4 → D18 (Pin 26) → GND - D4 (Left 2)
- Button 5 → D19 (Pin 27) → GND - E4 (Right 2)
- Button 6 → D20 (Pin 28) → GND - G3 (Left 3, lowest)
- Button 7 → D21 (Pin 29) → GND - A4 (Right 3)

**Potentiometer Controls:**
- A0: Global Brightness (0.5 - 1.0)
- A1: Global Decay/Sustain (multiplier)
- A2: Reverb Amount (0-100% dry/wet)
- A3: Reverb Size (0.7 - 0.95 room size)
- A4: LFO Rate (0.1 - 20 Hz vibrato speed)
- A5: LFO Depth (0 - 15% modulation amount)

**OLED Display (I2C):**
- SCL → D11 (Pin 12)
- SDA → D12 (Pin 13)
- Shows: Active notes, tuning, parameters in real-time

### Synthesis Parameters

Each note has frequency-dependent parameters for authentic kalimba character:

**Damping (sustain):**
- Low notes (G3, A3): 0.98 - Long sustain (8-10 seconds)
- Mid notes (D4, E4, G4): 0.92-0.96 - Medium sustain (5-8 seconds)
- High notes (A4, B4): 0.87-0.90 - Shorter sustain (2-5 seconds)

**Brightness (tone):**
- Low notes: 0.70 - Warm, mellow
- Mid notes: 0.76-0.82 - Balanced
- High notes: 0.85-0.90 - Bright, clear

**Non-linearity:** 0.1 (metallic kalimba character)

### Performance Characteristics

- **Polyphony:** All 7 voices can play simultaneously
- **CPU Usage:** ~12-15% (leaving 85% for future features)
- **Latency:** ~0.08ms (4-sample blocks @ 48kHz)
- **Memory:** ~95KB Flash, ~30KB RAM
- **Reverb:** Built-in ReverbSc for resonator box simulation

### Testing Kalimba

1. **Build and upload:** `./build-and-upload-kalimba.sh`
2. **Test each button:** Should hear corresponding note and see LED flash
3. **Verify tuning:** Use guitar tuner app to confirm frequencies
4. **Test polyphony:** Press multiple buttons simultaneously
5. **Adjust controls:** Verify brightness, decay, and reverb respond correctly

For complete wiring instructions, see **KALIMBA_WIRING.md**
