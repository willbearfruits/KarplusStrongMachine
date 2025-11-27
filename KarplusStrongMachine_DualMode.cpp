/*
 * KARPLUS-STRONG MACHINE - DUAL MODE
 * For Daisy Seed using libDaisy + DaisySP
 *
 * BOOT BUTTON: Toggle between modes
 * - Mode 1: AUTO-TRIGGER (plucks every 2 seconds, no pots needed)
 * - Mode 2: MANUAL TRIGGER (use pot A3 to trigger, full control)
 *
 * LED INDICATOR:
 * - Fast blink (4 Hz): Auto-trigger mode
 * - Slow blink (1 Hz): Manual mode
 * - Solid on trigger: Pluck happening
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

// Mode selection
enum TriggerMode {
    MODE_AUTO_TRIGGER,
    MODE_MANUAL
};

TriggerMode current_mode = MODE_AUTO_TRIGGER;  // Start in auto mode

// Button handling
Switch boot_button;
bool last_button_state = false;

// Auto-trigger timing
uint32_t auto_trigger_timer = 0;
const uint32_t AUTO_TRIGGER_INTERVAL = 96000;  // 2 seconds at 48kHz

// Manual trigger state
bool last_manual_trigger = false;
uint32_t manual_trigger_lockout = 0;
const uint32_t TRIGGER_LOCKOUT = 4800;  // 100ms

// LED blink timing
uint32_t led_timer = 0;
const uint32_t LED_BLINK_FAST = 6000;   // ~125ms (4 Hz blink in auto mode)
const uint32_t LED_BLINK_SLOW = 24000;  // ~500ms (1 Hz blink in manual mode)

// Control parameters (used in manual mode)
float pitch_freq = 220.0f;
float decay_amount = 0.9f;
float brightness = 0.5f;
float excitation_threshold = 0.8f;
float lfo_rate = 2.0f;
float lfo_depth = 0.5f;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Update controls (always read, used in manual mode)
    for (int i = 0; i < 6; i++) {
        controls[i].Process();
    }

    // Read control values
    float pot_pitch      = controls[0].Value();
    float pot_decay      = controls[1].Value();
    float pot_brightness = controls[2].Value();
    float pot_excite     = controls[3].Value();
    float pot_lfo_rate   = controls[4].Value();
    float pot_lfo_depth  = controls[5].Value();

    // Map controls
    pitch_freq = 50.0f * powf(40.0f, pot_pitch);
    decay_amount = pot_decay;
    brightness = pot_brightness;
    excitation_threshold = 0.6f + (pot_excite * 0.4f);
    lfo_rate = 0.1f * powf(200.0f, pot_lfo_rate);
    lfo_depth = pot_lfo_depth;

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);
    lfo_tremolo.SetFreq(lfo_rate * 0.7f);
    lfo_filter.SetFreq(lfo_rate * 0.4f);

    // Update string base parameters
    str.SetDamping(decay_amount);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        bool trigger = false;

        // MODE SELECTION: Auto-trigger or Manual
        if (current_mode == MODE_AUTO_TRIGGER) {
            // AUTO-TRIGGER MODE: Pluck every 2 seconds
            auto_trigger_timer++;
            if (auto_trigger_timer >= AUTO_TRIGGER_INTERVAL) {
                trigger = true;
                auto_trigger_timer = 0;
            }

            // Use fixed settings in auto mode (or use pot values, your choice)
            str.SetFreq(220.0f);  // Fixed A3 pitch

        } else {
            // MANUAL MODE: Use pot A3 for trigger
            bool trigger_active = pot_excite > excitation_threshold;
            bool trigger_edge = trigger_active && !last_manual_trigger && (manual_trigger_lockout == 0);
            last_manual_trigger = trigger_active;

            if (trigger_edge) {
                trigger = true;
                manual_trigger_lockout = TRIGGER_LOCKOUT;
            }
            if (manual_trigger_lockout > 0) {
                manual_trigger_lockout--;
            }

            // Use pot-controlled pitch in manual mode
            // Process LFOs
            float vibrato_sig = lfo_vibrato.Process();
            float tremolo_sig = lfo_tremolo.Process();
            float filter_sig = lfo_filter.Process();

            // Apply vibrato (pitch modulation)
            float pitch_mod = 1.0f + (vibrato_sig * 0.02f * lfo_depth);
            str.SetFreq(pitch_freq * pitch_mod);

            // Apply filter sweep
            float bright_mod = brightness + (filter_sig * 0.3f * lfo_depth);
            bright_mod = fclamp(bright_mod, 0.0f, 1.0f);
            str.SetBrightness(bright_mod);
        }

        // Process Karplus-Strong string
        float output = str.Process(trigger);

        // Apply tremolo (only in manual mode with LFO depth > 0)
        if (current_mode == MODE_MANUAL && lfo_depth > 0.01f) {
            float tremolo_sig = lfo_tremolo.Process();
            float amp_mod = 1.0f - (fabsf(tremolo_sig) * 0.5f * lfo_depth);
            output *= amp_mod;
        }

        // Remove DC offset
        output = dc_blocker.Process(output);

        // Boost volume slightly
        output *= 1.2f;

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

    // Initialize BOOT button (pin 28 on Daisy Seed)
    boot_button.Init(hw.GetPin(28), sample_rate / 48);

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

    // Start audio
    hw.StartAudio(AudioCallback);

    // Main loop - handle button and LED
    while(1) {
        // Debounce button
        boot_button.Debounce();

        // Check for button press (rising edge)
        if (boot_button.RisingEdge()) {
            // Toggle mode
            if (current_mode == MODE_AUTO_TRIGGER) {
                current_mode = MODE_MANUAL;
                auto_trigger_timer = 0;  // Reset auto timer
            } else {
                current_mode = MODE_AUTO_TRIGGER;
                manual_trigger_lockout = 0;  // Reset manual timer
            }
        }

        // LED indicator
        led_timer++;

        uint32_t blink_period = (current_mode == MODE_AUTO_TRIGGER) ? LED_BLINK_FAST : LED_BLINK_SLOW;

        // Blink LED to show mode
        bool led_state = (led_timer % blink_period) < (blink_period / 2);
        hw.SetLed(led_state);

        System::Delay(1);
    }
}
