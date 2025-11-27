/*
 * KARPLUS-STRONG PLUCKED STRING MACHINE - FIXED VERSION
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
 * LED: Blinks on each pluck
 *
 * FIXES APPLIED:
 * - Proper codec delay timing (100ms after Init, 50ms after StartAudio)
 * - Simplified initialization sequence
 * - Removed unnecessary 1-second delay
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Hardware
DaisySeed hw;

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
float trigger_speed = 2.0f;
float lfo_rate = 2.0f;
float lfo_depth = 0.5f;

// Trigger timing
uint32_t trigger_timer = 0;
uint32_t trigger_interval = 96000;

// LED timing
uint32_t led_timer = 0;
const uint32_t LED_ON_TIME = 4800;

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
    float pot_speed      = controls[3].Value();
    float pot_lfo_rate   = controls[4].Value();
    float pot_lfo_depth  = controls[5].Value();

    // Map controls with exponential scaling
    pitch_freq = 50.0f * powf(40.0f, pot_pitch);
    decay_amount = pot_decay;
    brightness = pot_brightness;
    trigger_speed = 0.1f * powf(100.0f, pot_speed);
    trigger_interval = (uint32_t)(trigger_speed * 48000.0f);
    lfo_rate = 0.1f * powf(200.0f, pot_lfo_rate);
    lfo_depth = pot_lfo_depth;

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);
    lfo_tremolo.SetFreq(lfo_rate * 0.7f);
    lfo_filter.SetFreq(lfo_rate * 0.4f);

    // Update string parameters
    str.SetDamping(decay_amount);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        // Auto-trigger logic
        trigger_timer++;
        bool trigger = false;
        if (trigger_timer >= trigger_interval) {
            trigger = true;
            trigger_timer = 0;
            led_timer = LED_ON_TIME;
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
    // This calls ConfigureAudio() which initializes the AK4556 codec
    // The codec init does a reset sequence: HIGH -> LOW -> HIGH with 1ms delays
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL TIMING FIX:
    // After AK4556 hardware reset, the codec needs ~10ms to stabilize
    // 100ms provides a safe margin for all codec initialization
    System::Delay(100);

    // Configure ADC for 6 analog inputs
    adc_config[0].InitSingle(seed::A0);
    adc_config[1].InitSingle(seed::A1);
    adc_config[2].InitSingle(seed::A2);
    adc_config[3].InitSingle(seed::A3);
    adc_config[4].InitSingle(seed::A4);
    adc_config[5].InitSingle(seed::A5);
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    // Initialize analog controls
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

    // Start audio - begins SAI DMA transfers
    hw.StartAudio(AudioCallback);

    // ADDITIONAL FIX:
    // Allow DMA and audio path to stabilize before main loop
    System::Delay(50);

    // Main loop - LED indicator
    while(1) {
        hw.SetLed(led_timer > 0);
        System::Delay(1);
    }
}
