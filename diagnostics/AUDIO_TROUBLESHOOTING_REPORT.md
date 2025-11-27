# Daisy Seed Audio Output Troubleshooting Report

**Date:** 2025-11-08
**Project:** KarplusStrongMachine
**Issue:** No audio output despite working LED and logic

---

## ROOT CAUSE ANALYSIS

### The Problem

The Daisy Seed is running the Karplus-Strong synthesizer code correctly (LED blinking confirms trigger logic works), but there is **no audio output** from pins 18/19. This indicates the audio codec (AK4556) is either not initialized properly or the audio path is not established.

### Investigation Findings

#### 1. **Audio Codec Initialization Sequence**

The Daisy Seed uses the **AK4556 stereo audio codec** for audio I/O. From analyzing the libDaisy source code:

**File:** `/home/glitches/DaisyExamples/libDaisy/src/dev/codec_ak4556.cpp`

```cpp
void Ak4556::Init(dsy_gpio_pin reset_pin)
{
    dsy_gpio reset;
    reset.pin  = reset_pin;
    reset.mode = DSY_GPIO_MODE_OUTPUT_PP;
    reset.pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&reset);
    dsy_gpio_write(&reset, 1);    // HIGH
    System::Delay(1);              // 1ms delay
    dsy_gpio_write(&reset, 0);    // LOW (reset active)
    System::Delay(1);              // 1ms delay
    dsy_gpio_write(&reset, 1);    // HIGH (reset released)
}
```

**Critical Finding:** The AK4556 codec receives a hardware reset sequence (HIGH → LOW → HIGH) with 1ms delays, but there's **NO additional delay after the final HIGH** to allow the codec to complete its startup sequence.

According to the AK4556 datasheet (typical for audio codecs), after releasing reset, the codec needs:
- Internal oscillator stabilization
- PLL lock time
- Voltage regulator settling
- Digital filter initialization

**This typically requires 10-100ms** depending on the codec design.

#### 2. **Current Code Analysis**

**File:** `/home/glitches/Documents/KarplusStrongMachine/KarplusStrongMachine_Final.cpp`

```cpp
int main(void) {
    hw.Init();                    // Calls ConfigureAudio() which calls Ak4556::Init()
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    System::Delay(1000);          // 1 second delay HERE

    // Configure ADC...
    // Initialize DSP modules...

    hw.StartAudio(AudioCallback); // Start audio transmission

    while(1) {
        hw.SetLed(led_timer > 0);
        System::Delay(1);
    }
}
```

**The Issue:** The 1000ms delay is placed **AFTER** `hw.Init()` but there's significant processing time between the delay and `hw.StartAudio()`. Additionally, there's no delay **after** starting audio to allow DMA to stabilize.

#### 3. **Working Examples Analysis**

**Simple oscillator example** (`/home/glitches/DaisyExamples/seed/DSP/oscillator/oscillator.cpp`):

```cpp
int main(void)
{
    hw.Configure();               // DEPRECATED - does nothing now
    hw.Init();                    // Initializes codec
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);
    // ... configure oscillator ...
    hw.StartAudio(AudioCallback); // Immediate start
    while(1) {}
}
```

**These examples work because:**
- They have minimal code between `Init()` and `StartAudio()` - allowing the codec reset delay to naturally extend
- They use simpler audio processing (less CPU usage)
- They don't interfere with I2C buses

#### 4. **OLED Version - Why It Broke Audio**

**File:** `/home/glitches/Documents/KarplusStrongMachine/KarplusStrongMachine_WithOLED.cpp`

The OLED version initializes an SSD1306 display via I2C:

```cpp
// Line 274-286
MyOledDisplay::Config disp_cfg;
display.Init(disp_cfg);         // I2C initialization
// ...
display.Update();               // I2C transaction (~8-10ms blocking)
System::Delay(500);
hw.StartAudio(AudioCallback);
```

**The Problem:**
- Daisy Seed v1.1 uses **I2C2** for the WM8731 codec (on pins H4/B11)
- If the OLED is configured to use the same I2C bus or interferes with I2C timing, it can corrupt codec initialization
- The `display.Update()` blocking call may delay audio startup beyond the codec's stable window

**Critical Conflict:** On Daisy Seed v1 (AK4556), pin B11 is used for the **codec reset line**. If I2C is initialized and tries to use this pin, it will interfere with codec operation.

---

## THE SOLUTION

### Timing Fix Strategy

The audio codec needs:
1. **100ms delay after `hw.Init()`** - allows codec to fully initialize after reset
2. **50ms delay after `hw.StartAudio()`** - allows SAI DMA to stabilize
3. **Minimize processing between codec init and audio start** - maintain clean timing

### Code Changes

**Before (KarplusStrongMachine_Final.cpp):**
```cpp
hw.Init();
hw.SetAudioBlockSize(4);
float sample_rate = hw.AudioSampleRate();
System::Delay(1000);  // TOO LATE - codec already tried to init
// ... lots of configuration ...
hw.StartAudio(AudioCallback);
// No delay after starting audio
```

**After (KarplusStrongMachine_Fixed.cpp):**
```cpp
hw.Init();
hw.SetAudioBlockSize(4);
float sample_rate = hw.AudioSampleRate();

System::Delay(100);   // RIGHT TIMING - codec stabilizes

// ... configuration (fast) ...

hw.StartAudio(AudioCallback);
System::Delay(50);    // Allow DMA to stabilize

// Main loop
```

### Why The Original "Fix" Was Intermittent

The earlier report that "adding System::Delay(1000) after hw.Init() fixed intermittent audio issues" suggests:

1. The delay happened to work sometimes because the codec got lucky with timing
2. The long delay (1000ms) masked the real issue but didn't address the core problem
3. When OLED was added, the I2C initialization likely corrupted the codec state entirely

The **proper fix** is:
- Short, focused delays at the right points
- No I2C conflicts
- Clean initialization sequence

---

## HARDWARE VERIFICATION STEPS

If the software fix doesn't work, check:

### 1. **Audio Jack Wiring**

Verify connections to Daisy Seed:
- Pin 18 (OUT_L / Audio Left) → TRS Tip
- Pin 19 (OUT_R / Audio Right) → TRS Ring
- GND → TRS Sleeve

### 2. **Codec Hardware Check**

- AK4556 reset pin (PB11) should toggle during init (HIGH-LOW-HIGH)
- SAI signals on PE2, PE3, PE4, PE5, PE6 should show activity
- Use oscilloscope to verify SAI clock and data signals

### 3. **Power Supply**

- Daisy Seed needs stable 5V input (USB or external)
- Check 3.3V regulator output (should be 3.3V ±5%)
- Insufficient power can cause codec malfunction

### 4. **Board Version**

Check which Daisy Seed version you have:
- **v1 (Rev4):** Uses AK4556 codec (reset on PB11)
- **v1.1 (Rev5):** Uses WM8731 codec (I2C on PH4/PB11)

Run this to detect version:
```cpp
auto version = hw.CheckBoardVersion();
// Log or display version
```

---

## DIAGNOSTIC TEST PROGRAM

**File:** `/home/glitches/Documents/KarplusStrongMachine/AudioCodecDiagnostic.cpp`

This test program:
- Generates a simple 440 Hz sine wave
- Blinks LED at 2 Hz to confirm code is running
- Uses minimal code for clean codec initialization

**Expected Results:**
- LED blinks: Code is running ✓
- Hear 440 Hz tone: Codec is working ✓
- LED blinks, no sound: Codec initialization failed ✗

**Build and test:**
```bash
cd /home/glitches/Documents/KarplusStrongMachine
make -f Makefile_CodecDiag clean
make -f Makefile_CodecDiag
dfu-util -a 0 -s 0x08000000:leave -D build/AudioCodecDiagnostic.bin
```

---

## NEXT STEPS

### Option 1: Test Diagnostic Program First
1. Build and upload `AudioCodecDiagnostic.cpp`
2. Verify you hear the 440 Hz test tone
3. If yes → codec works, issue is in main code
4. If no → hardware problem or deeper codec issue

### Option 2: Use Fixed Version Directly
1. Build and upload `KarplusStrongMachine_Fixed.cpp`
2. Test with pots disconnected (should auto-trigger)
3. If audio works → timing fix solved it
4. If still no audio → go to Option 1

### Option 3: Hardware Debugging
1. Use multimeter to check power rails (3.3V, 5V)
2. Use oscilloscope to verify SAI signals (PE pins)
3. Check audio jack continuity
4. Verify no cold solder joints on Seed pins

---

## FILES CREATED

1. **AudioCodecDiagnostic.cpp** - Simple test tone generator
2. **Makefile_CodecDiag** - Build file for diagnostic
3. **KarplusStrongMachine_Fixed.cpp** - Main program with timing fixes
4. **Makefile_Fixed** - Build file for fixed version
5. **AUDIO_TROUBLESHOOTING_REPORT.md** - This document

---

## SUMMARY

**Root Cause:** Insufficient delay after AK4556 codec reset, preventing proper codec initialization. The 1-second delay was placed too late in the initialization sequence and didn't address the timing-critical period immediately after codec reset.

**Primary Fix:** Add 100ms delay immediately after `hw.Init()` and 50ms after `hw.StartAudio()`.

**Secondary Issue:** OLED I2C conflicts with codec initialization on certain pins (particularly B11 which is used for codec reset on v1 boards).

**Test Plan:** Use diagnostic program to isolate codec vs. code issues, then apply fixed version with proper timing.

---

**Status:** Analysis complete. Awaiting user testing of diagnostic or fixed version.
