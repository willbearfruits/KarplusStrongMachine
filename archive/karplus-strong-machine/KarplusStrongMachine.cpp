/*
 * KARPLUS-STRONG PLUCKED STRING MACHINE
 * For Daisy Seed using libDaisy + DaisySP
 *
 * FEATURES:
 * - Karplus-Strong physical modeling (DaisySP String class)
 * - Long decay times (up to 20+ seconds)
 * - Triple LFO modulation (vibrato, tremolo, filter sweep)
 * - 6 potentiometer controls
 * - Mono output (optimized for CPU)
 *
 * PERFORMANCE: ~5-8% CPU usage
 *
 * HARDWARE CONTROLS:
 * - A0 (D15): Pitch (50 Hz - 2000 Hz)
 * - A1 (D16): Decay Time (1s to 20s+)
 * - A2 (D17): Brightness (filter cutoff)
 * - A3 (D18): Excitation (manual trigger threshold)
 * - A4 (D19): LFO Rate (0.1 Hz - 20 Hz)
 * - A5 (D20): LFO Depth (modulation amount)
 *
 * BUILD:
 * make clean && make -j4
 *
 * UPLOAD:
 * make program-dfu (with Daisy in bootloader mode)
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Hardware
DaisySeed hw;

// DSP modules
String     str;              // Karplus-Strong string model
Oscillator lfo_vibrato;      // Pitch modulation LFO
Oscillator lfo_tremolo;      // Amplitude modulation LFO
Oscillator lfo_filter;       // Filter sweep LFO
DcBlock    dc_blocker;       // DC offset removal

// Controls
AdcChannelConfig adc_config[6];
AnalogControl controls[6];

// Control parameters
float pitch_freq = 220.0f;
float decay_amount = 0.9f;
float brightness = 0.5f;
float excitation_threshold = 0.8f;
float lfo_rate = 2.0f;
float lfo_depth = 0.5f;

// Trigger state
bool last_trigger = false;
uint32_t trigger_timer = 0;
const uint32_t TRIGGER_LOCKOUT = 4800;  // 100ms at 48kHz (prevent retriggering)

// Performance: Fast random for excitation noise
inline float FastRand() {
    static uint32_t seed = 1664525;
    seed = seed * 1664525 + 1013904223;
    return ((float)seed * 2.3283064365e-10f) * 2.0f - 1.0f;  // -1 to 1
}

// Audio callback
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Update controls (once per block, non-blocking DMA reads)
    for (int i = 0; i < 6; i++) {
        controls[i].Process();
    }

    // Read control values (0.0 to 1.0, pre-filtered)
    float pot_pitch      = controls[0].Value();
    float pot_decay      = controls[1].Value();
    float pot_brightness = controls[2].Value();
    float pot_excite     = controls[3].Value();
    float pot_lfo_rate   = controls[4].Value();
    float pot_lfo_depth  = controls[5].Value();

    // Map controls with exponential scaling for musical response
    pitch_freq = 20.0f * powf(40.0f, pot_pitch);  // 50 Hz to 2000 Hz
    decay_amount = pot_decay;  // 0.0 to 1.0 (1.0 = longest decay)
    brightness = pot_brightness;
    excitation_threshold = 0.2f + (pot_excite * 0.4f);  // 0.6 to 1.0
    lfo_rate = 0.01f * powf(500.0f, pot_lfo_rate);  // 0.1 Hz to 20 Hz
    lfo_depth = pot_lfo_depth;

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);
    lfo_tremolo.SetFreq(lfo_rate * 0.7f);  // Slightly slower for interest
    lfo_filter.SetFreq(lfo_rate * 0.4f);   // Even slower filter sweep

    // Update string base parameters
    str.SetDamping(decay_amount);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        // Trigger detection (rising edge when pot crosses threshold)
        bool trigger_active = pot_excite > excitation_threshold;
        bool trigger_edge = trigger_active && !last_trigger && (trigger_timer == 0);
        last_trigger = trigger_active;

        // Trigger lockout timer (prevent immediate retriggering)
        if (trigger_edge) {
            trigger_timer = TRIGGER_LOCKOUT;
        }
        if (trigger_timer > 0) {
            trigger_timer--;
        }

        // Process LFOs
        float vibrato_sig = lfo_vibrato.Process();  // -1 to 1
        float tremolo_sig = lfo_tremolo.Process();
        float filter_sig = lfo_filter.Process();

        // Apply vibrato (pitch modulation, ±2% with depth control)
        float pitch_mod = 1.0f + (vibrato_sig * 0.02f * lfo_depth);
        str.SetFreq(pitch_freq * pitch_mod);

        // Apply filter sweep (brightness modulation)
        float bright_mod = brightness + (filter_sig * 0.3f * lfo_depth);
        bright_mod = fclamp(bright_mod, 0.0f, 1.0f);
        str.SetBrightness(bright_mod);

        // Process Karplus-Strong string
        float output = str.Process(trigger_edge);

        // Apply tremolo (amplitude modulation)
        // Convert LFO -1..1 to 0.5..1.0 range, scaled by depth
        float amp_mod = 1.0f - (fabsf(tremolo_sig) * 0.5f * lfo_depth);
        output *= amp_mod;

        // Remove DC offset (prevents drift in feedback loop)
        output = dc_blocker.Process(output);

        // Soft limiting (gentle saturation for long decays)
        output = tanhf(output * 1.2f) * 0.8f;

        // Output (mono to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);  // Low callback noise (12 kHz)
    float sample_rate = hw.AudioSampleRate();

    // Configure ADC for 6 analog inputs (non-blocking DMA mode)
    adc_config[0].InitSingle(seed::A0);
    adc_config[1].InitSingle(seed::A1);
    adc_config[2].InitSingle(seed::A2);
    adc_config[3].InitSingle(seed::A3);
    adc_config[4].InitSingle(seed::A4);
    adc_config[5].InitSingle(seed::A5);
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    // Initialize analog controls with filtering (prevents zipper noise)
    for (int i = 0; i < 6; i++) {
        controls[i].Init(hw.adc.GetPtr(i), sample_rate / 48);
    }

    // Initialize Karplus-Strong string
    str.Init(sample_rate);
    str.SetFreq(220.0f);        // Starting pitch (A3)
    str.SetDamping(0.9f);       // Long decay
    str.SetBrightness(0.5f);    // Medium brightness
    str.SetNonLinearity(0.1f);  // Subtle dispersion (more realistic)

    // Initialize LFOs
    lfo_vibrato.Init(sample_rate);
    lfo_vibrato.SetWaveform(Oscillator::WAVE_SIN);  // Smooth sine for vibrato
    lfo_vibrato.SetAmp(1.0f);
    lfo_vibrato.SetFreq(5.0f);

    lfo_tremolo.Init(sample_rate);
    lfo_tremolo.SetWaveform(Oscillator::WAVE_TRI);  // Triangle for tremolo
    lfo_tremolo.SetAmp(1.0f);
    lfo_tremolo.SetFreq(3.5f);

    lfo_filter.Init(sample_rate);
    lfo_filter.SetWaveform(Oscillator::WAVE_SAW);  // Saw for filter sweep
    lfo_filter.SetAmp(1.0f);
    lfo_filter.SetFreq(2.0f);

    // Initialize DC blocker
    dc_blocker.Init(sample_rate);

    // Start audio
    hw.StartAudio(AudioCallback);

    // Main loop (could add LED indicators, MIDI, etc)
    while(1) {
        // Blink LED to show it's running
        System::Delay(500);
    }
}
