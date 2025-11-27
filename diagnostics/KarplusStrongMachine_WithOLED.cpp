/*
 * KARPLUS-STRONG PLUCKED STRING MACHINE - FINAL VERSION WITH OLED
 * For Daisy Seed using libDaisy + DaisySP
 *
 * AUTO-TRIGGER MODE with speed control
 *
 * CONTROLS:
 * - A0: Pitch (50 Hz - 2000 Hz)
 * - A1: Decay Time (1s to 20s+)
 * - A2: Brightness (filter cutoff)
 * - A3: TRIGGER SPEED (0.1 sec - 10 sec interval)
 * - A4: LFO Rate (0.1 Hz - 20 Hz)
 * - A5: LFO Depth (modulation amount)
 *
 * DISPLAY:
 * - 0.96" OLED (SSD1306, I2C, 128x64)
 * - SCL: Pin 12, SDA: Pin 13
 * - Shows real-time parameter values
 *
 * LED: Blinks on each pluck
 */

#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"
#include <cstdio>

using namespace daisy;
using namespace daisysp;

// Type alias for OLED display
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

// Hardware
DaisySeed hw;
MyOledDisplay display;

// DSP modules
String     str;
Oscillator lfo_vibrato;
Oscillator lfo_tremolo;
Oscillator lfo_filter;
DcBlock    dc_blocker;

// Controls
AdcChannelConfig adc_config[6];
AnalogControl controls[6];

// Control parameters
float pitch_freq = 220.0f;
float decay_amount = 0.9f;
float brightness = 0.5f;
float trigger_speed = 2.0f;  // Seconds between plucks
float lfo_rate = 2.0f;
float lfo_depth = 0.5f;

// Trigger timing
uint32_t trigger_timer = 0;
uint32_t trigger_interval = 96000;  // Start at 2 seconds (48kHz × 2)

// LED timing
uint32_t led_timer = 0;
const uint32_t LED_ON_TIME = 4800;  // 100ms LED flash on trigger

// Display update timing
uint32_t last_display_update = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 100;  // 100ms = 10 Hz update rate

// Previous values for change detection
float prev_pitch = -1.0f;
float prev_decay = -1.0f;
float prev_brightness = -1.0f;
float prev_trigger_speed = -1.0f;
float prev_lfo_rate = -1.0f;
float prev_lfo_depth = -1.0f;

void UpdateDisplay() {
    // Only update if values have changed significantly
    bool needs_update = false;

    if (fabs(pitch_freq - prev_pitch) > 1.0f ||
        fabs(decay_amount - prev_decay) > 0.01f ||
        fabs(brightness - prev_brightness) > 0.01f ||
        fabs(trigger_speed - prev_trigger_speed) > 0.1f ||
        fabs(lfo_rate - prev_lfo_rate) > 0.1f ||
        fabs(lfo_depth - prev_lfo_depth) > 0.01f ||
        prev_pitch < 0.0f) {  // First run
        needs_update = true;
    }

    if (!needs_update) return;

    // Clear display
    display.Fill(false);
    char str[32];

    // Line 1: Pitch (large font, main parameter)
    sprintf(str, "%.1f Hz", pitch_freq);
    display.SetCursor(0, 0);
    display.WriteString(str, Font_11x18, true);

    // Line 2: Decay time
    sprintf(str, "Decay: %.2f", decay_amount);
    display.SetCursor(0, 20);
    display.WriteString(str, Font_7x10, true);

    // Line 3: Trigger speed
    sprintf(str, "Trig: %.1fs", trigger_speed);
    display.SetCursor(0, 32);
    display.WriteString(str, Font_7x10, true);

    // Line 4: LFO rate and depth (combined)
    sprintf(str, "LFO:%.1fHz D:%.0f%%", lfo_rate, lfo_depth * 100.0f);
    display.SetCursor(0, 44);
    display.WriteString(str, Font_7x10, true);

    // Line 5: Brightness indicator
    sprintf(str, "Bright: %.0f%%", brightness * 100.0f);
    display.SetCursor(0, 56);
    display.WriteString(str, Font_6x8, true);

    // Push buffer to display (blocking ~8-10ms)
    display.Update();

    // Cache current values
    prev_pitch = pitch_freq;
    prev_decay = decay_amount;
    prev_brightness = brightness;
    prev_trigger_speed = trigger_speed;
    prev_lfo_rate = lfo_rate;
    prev_lfo_depth = lfo_depth;
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Update controls (once per block)
    for (int i = 0; i < 6; i++) {
        controls[i].Process();
    }

    // Read control values
    float pot_pitch      = controls[0].Value();
    float pot_decay      = controls[1].Value();
    float pot_brightness = controls[2].Value();
    float pot_speed      = controls[3].Value();  // A3 = trigger speed
    float pot_lfo_rate   = controls[4].Value();
    float pot_lfo_depth  = controls[5].Value();

    // Map controls with exponential scaling for musical response
    pitch_freq = 50.0f * powf(40.0f, pot_pitch);  // 50 Hz to 2000 Hz
    decay_amount = pot_decay;  // 0.0 to 1.0
    brightness = pot_brightness;

    // A3: Trigger speed (0.1 sec to 10 sec interval)
    // Exponential: slow at CCW, fast at CW
    trigger_speed = 0.1f * powf(100.0f, pot_speed);  // 0.1s to 10s
    trigger_interval = (uint32_t)(trigger_speed * 48000.0f);  // Convert to samples

    lfo_rate = 0.1f * powf(200.0f, pot_lfo_rate);  // 0.1 Hz to 20 Hz
    lfo_depth = pot_lfo_depth;

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);
    lfo_tremolo.SetFreq(lfo_rate * 0.7f);
    lfo_filter.SetFreq(lfo_rate * 0.4f);

    // Update string parameters
    str.SetDamping(decay_amount);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        // Auto-trigger based on A3 speed control
        trigger_timer++;
        bool trigger = false;
        if (trigger_timer >= trigger_interval) {
            trigger = true;
            trigger_timer = 0;
            led_timer = LED_ON_TIME;  // Flash LED
        }

        // Decrement LED timer
        if (led_timer > 0) {
            led_timer--;
        }

        // Process LFOs
        float vibrato_sig = lfo_vibrato.Process();
        float tremolo_sig = lfo_tremolo.Process();
        float filter_sig = lfo_filter.Process();

        // Apply vibrato (pitch modulation)
        float pitch_mod = 1.0f + (vibrato_sig * 0.02f * lfo_depth);
        str.SetFreq(pitch_freq * pitch_mod);

        // Apply filter sweep (brightness modulation)
        float bright_mod = brightness + (filter_sig * 0.3f * lfo_depth);
        bright_mod = fclamp(bright_mod, 0.0f, 1.0f);
        str.SetBrightness(bright_mod);

        // Process Karplus-Strong string
        float output = str.Process(trigger);

        // Apply tremolo (amplitude modulation)
        float amp_mod = 1.0f - (fabsf(tremolo_sig) * 0.5f * lfo_depth);
        output *= amp_mod;

        // Remove DC offset
        output = dc_blocker.Process(output);

        // Boost volume
        output *= 1.3f;

        // Soft limiting
        output = tanhf(output * 1.2f) * 0.8f;

        // Output (mono to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL: Delay to allow audio codec to initialize
    System::Delay(1000);

    // Configure ADC for 6 analog inputs
    adc_config[0].InitSingle(seed::A0);
    adc_config[1].InitSingle(seed::A1);
    adc_config[2].InitSingle(seed::A2);
    adc_config[3].InitSingle(seed::A3);
    adc_config[4].InitSingle(seed::A4);
    adc_config[5].InitSingle(seed::A5);
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    // Initialize analog controls with filtering
    for (int i = 0; i < 6; i++) {
        controls[i].Init(hw.adc.GetPtr(i), sample_rate / 48);
    }

    // Initialize Karplus-Strong string
    str.Init(sample_rate);
    str.SetFreq(220.0f);
    str.SetDamping(0.9f);
    str.SetBrightness(0.5f);
    str.SetNonLinearity(0.1f);

    // Initialize LFOs
    lfo_vibrato.Init(sample_rate);
    lfo_vibrato.SetWaveform(Oscillator::WAVE_SIN);
    lfo_vibrato.SetAmp(1.0f);
    lfo_vibrato.SetFreq(5.0f);

    lfo_tremolo.Init(sample_rate);
    lfo_tremolo.SetWaveform(Oscillator::WAVE_TRI);
    lfo_tremolo.SetAmp(1.0f);
    lfo_tremolo.SetFreq(3.5f);

    lfo_filter.Init(sample_rate);
    lfo_filter.SetWaveform(Oscillator::WAVE_SAW);
    lfo_filter.SetAmp(1.0f);
    lfo_filter.SetFreq(2.0f);

    // Initialize DC blocker
    dc_blocker.Init(sample_rate);

    // Initialize OLED display (I2C, pins 12/13)
    MyOledDisplay::Config disp_cfg;
    // Use default config (I2C1, pins 12/13, 1MHz, address 0x3C)
    display.Init(disp_cfg);

    // Show startup splash screen
    display.Fill(false);
    display.SetCursor(0, 20);
    display.WriteString("Karplus-Strong", Font_11x18, true);
    display.SetCursor(0, 40);
    display.WriteString("Synthesizer", Font_7x10, true);
    display.Update();
    System::Delay(500);  // Show splash for 500ms

    // Start audio
    hw.StartAudio(AudioCallback);

    // Main loop - LED indicator and display updates
    while(1) {
        uint32_t now = System::GetNow();

        // Time-sliced display updates (10 Hz)
        if (now - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
            UpdateDisplay();
            last_display_update = now;
        }

        // LED on when trigger happens (led_timer > 0)
        hw.SetLed(led_timer > 0);

        // Small delay to prevent CPU spinning
        System::Delay(1);
    }
}
