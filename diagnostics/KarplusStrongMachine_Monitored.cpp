/*
 * KARPLUS-STRONG PLUCKED STRING MACHINE - MONITORED VERSION
 * For Daisy Seed using libDaisy + DaisySP
 *
 * AUTO-TRIGGER MODE with speed control + USB Serial Monitoring
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
 * Monitor: screen /dev/ttyACM0 115200
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

// Monitoring
uint32_t callback_count = 0;
uint32_t status_print_timer = 0;
bool audio_running = false;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    callback_count++;

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
        // Auto-trigger
        trigger_timer++;
        bool trigger = false;
        if (trigger_timer >= trigger_interval) {
            trigger = true;
            trigger_timer = 0;
            led_timer = LED_ON_TIME;
        }

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
    char buffer[128];

    // Initialize hardware
    hw.Init();

    // Start USB serial FIRST
    hw.StartLog(true);

    // LED on during init
    hw.SetLed(true);

    // CRITICAL: Extended delay for codec initialization
    System::Delay(1000);  // 1 second delay

    hw.PrintLine("===========================================");
    hw.PrintLine("KARPLUS-STRONG MACHINE - MONITORED");
    hw.PrintLine("===========================================");
    hw.PrintLine("");

    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    sprintf(buffer, "Sample Rate: %.0f Hz", sample_rate);
    hw.PrintLine(buffer);
    sprintf(buffer, "Block Size: %d samples", (int)hw.AudioBlockSize());
    hw.PrintLine(buffer);
    hw.PrintLine("");

    // Configure ADC for 6 analog inputs
    hw.PrintLine("Configuring ADC channels...");
    adc_config[0].InitSingle(seed::A0);
    adc_config[1].InitSingle(seed::A1);
    adc_config[2].InitSingle(seed::A2);
    adc_config[3].InitSingle(seed::A3);
    adc_config[4].InitSingle(seed::A4);
    adc_config[5].InitSingle(seed::A5);
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();
    hw.PrintLine("ADC started");

    // Initialize analog controls with filtering
    hw.PrintLine("Initializing analog controls...");
    for (int i = 0; i < 6; i++) {
        controls[i].Init(hw.adc.GetPtr(i), sample_rate / 48);
    }
    hw.PrintLine("Analog controls ready");

    // Initialize Karplus-Strong string
    hw.PrintLine("Initializing String DSP...");
    str.Init(sample_rate);
    str.SetFreq(220.0f);
    str.SetDamping(0.9f);
    str.SetBrightness(0.5f);
    str.SetNonLinearity(0.1f);
    hw.PrintLine("String initialized");

    // Initialize LFOs
    hw.PrintLine("Initializing LFOs...");
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
    hw.PrintLine("LFOs initialized");

    // Initialize DC blocker
    hw.PrintLine("Initializing DC blocker...");
    dc_blocker.Init(sample_rate);
    hw.PrintLine("DC blocker ready");

    hw.PrintLine("");
    hw.PrintLine("Starting audio engine...");

    // Start audio
    hw.StartAudio(AudioCallback);
    audio_running = true;

    hw.PrintLine("AUDIO RUNNING!");
    hw.PrintLine("");
    hw.PrintLine("Controls:");
    hw.PrintLine("  A0: Pitch (50-2000 Hz)");
    hw.PrintLine("  A1: Decay Time (1-20s)");
    hw.PrintLine("  A2: Brightness");
    hw.PrintLine("  A3: Trigger Speed (0.1-10s)");
    hw.PrintLine("  A4: LFO Rate (0.1-20 Hz)");
    hw.PrintLine("  A5: LFO Depth");
    hw.PrintLine("");
    hw.PrintLine("Status updates every 5 seconds...");
    hw.PrintLine("===========================================");
    hw.PrintLine("");

    // LED off after init
    hw.SetLed(false);

    // Main loop - LED indicator and status monitoring
    while(1) {
        status_print_timer++;

        // Print status every 5 seconds
        if (status_print_timer >= 5000) {
            sprintf(buffer, "Callbacks: %lu | Pitch: %.0f Hz | Trigger: %.1fs",
                    callback_count, pitch_freq, trigger_speed);
            hw.PrintLine(buffer);
            status_print_timer = 0;
        }

        // LED on when trigger happens
        hw.SetLed(led_timer > 0);
        System::Delay(1);
    }
}
