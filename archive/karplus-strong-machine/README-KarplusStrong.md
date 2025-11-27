# Karplus-Strong Plucked String Machine

A physical modeling synthesizer for Daisy Seed featuring Karplus-Strong algorithm with triple LFO modulation and long decay times.

**Performance:** 5-8% CPU usage, 90KB binary, 20+ second decay capability

---

## Features

- **Karplus-Strong Algorithm** - Physics-based plucked string synthesis
- **Long Decay Times** - Up to 20+ seconds sustain
- **Triple LFO Modulation** - Simultaneous vibrato, tremolo, and filter sweep
- **Mono Output** - CPU-optimized for maximum expressiveness
- **6 Hardware Controls** - Full real-time parameter control
- **DC Blocking** - Clean output without drift
- **Trigger Lockout** - Prevents unwanted retriggering

---

## Quick Start

```bash
cd ~/Documents/KarplusStrongMachine/
./build-and-upload.sh
```

See **QUICKSTART.txt** for immediate usage guide.

---

## Hardware Requirements

### Essential
- Daisy Seed board
- 6 potentiometers (10kΩ linear recommended)
- Audio output (line level speakers or headphone amp)
- Power (USB or 9V DC)

### Wiring

**Audio Output:**
```
Pin 22 (Audio Out L) → Left channel
Pin 23 (Audio Out R) → Right channel
GND → Audio ground
```

**Control Pots:**
```
A0 (Pin 15) → Pitch
A1 (Pin 16) → Decay Time
A2 (Pin 17) → Brightness
A3 (Pin 18) → Excitation
A4 (Pin 19) → LFO Rate
A5 (Pin 20) → LFO Depth
```

**Pot Wiring (each):**
```
Pin 1 (CCW) → GND
Pin 2 (Wiper) → A0-A5 analog input
Pin 3 (CW) → 3.3V (⚠️ NOT 5V!)
```

---

## Control Reference

### A0 - Pitch (50 Hz - 2000 Hz)
Fundamental frequency of the plucked string.

- **0%**: 50 Hz (sub-bass, very low)
- **25%**: 130 Hz (C3)
- **50%**: 330 Hz (E4)
- **75%**: 830 Hz (G#5)
- **100%**: 2000 Hz (very bright)

**Exponential response** for musical tuning across the range.

### A1 - Decay Time (1s to 20s+)
Controls how long the string sustains after being plucked.

- **0%**: ~1 second (short pluck)
- **50%**: ~5 seconds (medium sustain)
- **100%**: 20+ seconds (nearly infinite)

**Technical:** Maps to string damping coefficient (0.0 to 1.0 in DaisySP String class).

### A2 - Brightness (0% - 100%)
Filter cutoff controlling the tone from dark to bright.

- **0%**: Dull, muffled (bass-like)
- **50%**: Balanced (acoustic guitar)
- **100%**: Bright, metallic (sitar-like)

**Technical:** Controls lowpass filter cutoff in the Karplus-Strong feedback loop.

### A3 - Excitation (Manual Trigger)
Triggers the string pluck when turned past threshold (60%).

- **< 60%**: No trigger (string decays naturally)
- **> 60%**: Pluck string (rising edge detection)

**Usage:**
- Quick up/down motion for clean single plucks
- Hold above 60% for repeated triggering
- 100ms lockout prevents too-fast retriggering

**Technical:** Rising edge detector with 100ms cooldown (4800 samples at 48kHz).

### A4 - LFO Rate (0.1 Hz - 20 Hz)
Speed of all three LFO modulations.

- **0%**: 0.1 Hz (very slow, 10 second cycle)
- **25%**: 1 Hz (slow movement)
- **50%**: 5 Hz (vibrato range)
- **75%**: 10 Hz (fast warble)
- **100%**: 20 Hz (extreme, almost ring mod)

**Exponential response** for musical LFO control.

### A5 - LFO Depth (0% - 100%)
Amount of LFO modulation applied to all three destinations.

- **0%**: No modulation (static sound)
- **50%**: Subtle movement
- **100%**: Extreme modulation

**Simultaneously controls:**
- Vibrato amount (pitch modulation)
- Tremolo amount (amplitude modulation)
- Filter sweep amount (brightness modulation)

---

## LFO System Explained

The synthesizer features **3 independent LFOs** running simultaneously:

### LFO 1: Vibrato (Pitch Modulation)
- **Waveform:** Sine (smooth)
- **Rate:** Controlled by A4
- **Depth:** ±2% pitch variation × A5
- **Effect:** Musical vibrato, adds life to sustained notes

### LFO 2: Tremolo (Amplitude Modulation)
- **Waveform:** Triangle
- **Rate:** 0.7× A4 (slightly slower than vibrato)
- **Depth:** 0-50% volume variation × A5
- **Effect:** Pulsing amplitude, rhythmic movement

### LFO 3: Filter Sweep (Brightness Modulation)
- **Waveform:** Sawtooth
- **Rate:** 0.4× A4 (slowest of the three)
- **Depth:** ±30% brightness variation × A5
- **Effect:** Gradual tonal changes, evolving timbre

**Design:** Different waveforms and rates create complex, evolving textures without sounding like a single LFO.

---

## Karplus-Strong Algorithm

### How It Works

Karplus-Strong synthesis simulates plucked strings using:

1. **Delay Line** - Stores one period of the waveform
2. **Excitation** - Noise burst "plucks" the string
3. **Feedback Loop** - Delayed signal feeds back into itself
4. **Lowpass Filter** - Simulates string damping (brightness control)
5. **Decay Coefficient** - Controls sustain time

**Physical model:** Vibrating string with damping and dispersion.

### Implementation (DaisySP String Class)

This synth uses the **`daisysp::String`** class:
- Production-ready Karplus-Strong implementation
- Ported from Mutable Instruments Rings (Emilie Gillet)
- Optimized for STM32H750 (ARM Cortex-M7)
- Built-in brightness, damping, non-linearity controls

**Benefits:**
- Band-limited (no aliasing)
- Tunable dispersion (non-linearity)
- Efficient (1-2% CPU per voice)
- Realistic plucked string behavior

---

## Performance & Memory

### CPU Usage
**Estimated: 5-8% at 48kHz**

Breakdown:
- String class (K-S): ~2%
- 3× Oscillator (LFOs): ~0.3%
- DcBlock: ~0.1%
- Analog controls: ~0.5%
- Audio processing: ~1%
- Overhead: ~1%

**Remaining:** 92-95% CPU available for effects/expansion!

### Memory Usage

**Flash:** 91,220 bytes / 128 KB (69.6%)
- Room for 40KB more code

**SRAM:** 18,764 bytes / 512 KB (3.6%)
- Mostly delay line buffers
- Could fit 25+ voices (memory-wise)

**RAM_D2:** 16,704 bytes / 288 KB (5.7%)
- DMA buffers

### Latency
**~0.083ms** with block size = 4 (48 samples at 48kHz)

---

## Technical Details

### Audio Processing Chain

```
Excitation (noise burst)
    ↓
String Class (Karplus-Strong)
    ├─ Delay line (pitch-dependent)
    ├─ Lowpass filter (brightness)
    └─ Feedback loop (decay)
    ↓
LFO Modulation
    ├─ Vibrato (pitch)
    ├─ Tremolo (amplitude)
    └─ Filter sweep (brightness)
    ↓
DC Blocker
    ↓
Soft Saturation (tanh)
    ↓
Output (mono → stereo)
```

### Control Processing

- **Non-blocking ADC** via DMA (zero audio callback overhead)
- **Filtered inputs** via `AnalogControl` class (no zipper noise)
- **Control rate:** ~1kHz (once per audio block)
- **Exponential scaling** for pitch and LFO rate (musical response)

### Trigger System

**Rising edge detection:**
```cpp
bool trigger_active = pot_excite > threshold;
bool trigger_edge = trigger_active && !last_trigger && (timer == 0);
```

**Lockout timer:**
- 100ms cooldown (4800 samples at 48kHz)
- Prevents accidental double triggers
- Allows intentional retriggering if pot held high

---

## Preset Sounds

### Acoustic Guitar
```
A0: 40%  (165 Hz)
A1: 50%  (medium decay)
A2: 60%  (bright)
A3: trigger
A4: 20%  (slow LFO)
A5: 30%  (subtle mod)
```
Natural acoustic guitar pluck with gentle vibrato.

### Bass Pluck
```
A0: 10%  (82 Hz)
A1: 30%  (short)
A2: 30%  (dark)
A3: trigger
A4: 10%  (very slow)
A5: 10%  (minimal)
```
Punchy bass guitar pluck, short and tight.

### Sitar / Drone
```
A0: 30%  (130 Hz)
A1: 90%  (very long)
A2: 80%  (bright)
A3: 70%  (sustained)
A4: 50%  (medium)
A5: 70%  (heavy mod)
```
Long sustaining drone with heavy modulation, sitar-like.

### Bell / Metallic
```
A0: 80%  (1000 Hz)
A1: 70%  (long)
A2: 90%  (very bright)
A3: trigger quickly
A4: 30%  (medium)
A5: 40%  (moderate)
```
Bright, metallic bell tone with shimmering modulation.

### Wobble Bass
```
A0: 15%  (100 Hz)
A1: 80%  (long)
A2: 40%  (mid)
A3: trigger
A4: 70%  (fast)
A5: 90%  (extreme)
```
Dubstep-style wobble bass with heavy LFO.

---

## Build Instructions

### Prerequisites
```bash
# ARM toolchain
arm-none-eabi-gcc --version

# DFU utility
dfu-util --version

# Libraries (should exist)
ls ~/DaisyExamples/libDaisy
ls ~/DaisyExamples/DaisySP
```

### Compile
```bash
cd ~/Documents/KarplusStrongMachine/
make clean && make -j4
```

### Upload
1. Put Daisy in bootloader mode:
   - Hold BOOT button
   - Press RESET button
   - Release BOOT button
   - LED pulses slowly

2. Upload:
```bash
make program-dfu
# or
./upload.sh
```

3. Press RESET to start

### One-Step Build & Upload
```bash
./build-and-upload.sh
```

---

## Troubleshooting

### No Sound
1. Turn up A3 to trigger string
2. Check audio wiring (Pins 22, 23, GND)
3. Verify Daisy has power
4. Press RESET button

### Won't Trigger
1. Turn A3 past 60% threshold
2. Try quick up/down motion
3. Check pot is wired correctly (3.3V, not 5V)
4. Wait 100ms between triggers (lockout period)

### Unwanted Retriggering
1. Turn A3 down after initial trigger
2. String will decay naturally
3. Lockout timer prevents too-fast triggers

### No Vibrato/Modulation
1. Turn up A5 (LFO depth)
2. Adjust A4 (LFO rate) to hear effect
3. LFO may be very slow - wait for full cycle

### Sound Too Bright/Harsh
1. Turn down A2 (brightness)
2. Lower A0 (pitch)
3. Reduce A5 (LFO depth)

### Decay Too Short
1. Turn up A1 (decay time)
2. At 100%, decay is 20+ seconds

---

## Future Enhancements

With 92% CPU remaining, possible additions:

**Effects:**
- [ ] Reverb (10-15% CPU)
- [ ] Delay/echo (5-8% CPU)
- [ ] Chorus (3-5% CPU)

**Synthesis:**
- [ ] Polyphony (4-6 voices possible)
- [ ] Multiple string models
- [ ] Per-voice tuning/detuning

**Control:**
- [ ] MIDI input (pitch CV)
- [ ] Gate/trigger input
- [ ] CV control of parameters
- [ ] Preset storage (flash memory)

**Display:**
- [ ] OLED showing parameters
- [ ] LED indicators per control

---

## Technical Resources

### DaisySP Classes Used
- `String` - Karplus-Strong implementation
- `Oscillator` - LFO waveform generators
- `AnalogControl` - Filtered control inputs
- `DcBlock` - DC offset removal

### References
- DaisySP Docs: https://electro-smith.github.io/DaisySP/
- libDaisy Docs: https://electro-smith.github.io/libDaisy/
- Daisy Forum: https://forum.electro-smith.com/
- Mutable Instruments Rings: https://mutable-instruments.net/modules/rings/

---

## Credits

**Algorithm:** Karplus-Strong (1983)
**String Class:** DaisySP (ported from MI Rings by Emilie Gillet)
**Platform:** Daisy Seed by Electrosmith
**Created:** 2025-10-29

---

## License

Open source - use and modify as you wish!

---

**Enjoy creating evolving plucked string textures! 🎸**
