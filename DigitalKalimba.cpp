/*
 * DIGITAL KALIMBA - 7-Button Karplus-Strong Instrument
 * For Daisy Seed using libDaisy + DaisySP
 *
 * Based on KarplusStrongMachine_Final.cpp
 * Modified to create a playable 7-note kalimba instrument
 *
 * TUNING: G Major Pentatonic (traditional kalimba tuning)
 *   Button 1 (center): G4 (392 Hz)
 *   Button 2 (left 1):  A3 (220 Hz)
 *   Button 3 (right 1): B4 (493.88 Hz)
 *   Button 4 (left 2):  D4 (293.66 Hz)
 *   Button 5 (right 2): E4 (329.63 Hz)
 *   Button 6 (left 3):  G3 (196 Hz)
 *   Button 7 (right 3): A4 (440 Hz)
 *
 * BUTTON WIRING:
 *   Button 1 → D15 (Pin 23) → GND
 *   Button 2 → D16 (Pin 24) → GND
 *   Button 3 → D17 (Pin 25) → GND
 *   Button 4 → D18 (Pin 26) → GND
 *   Button 5 → D19 (Pin 27) → GND
 *   Button 6 → D20 (Pin 28) → GND
 *   Button 7 → D21 (Pin 29) → GND
 *   (Internal pull-ups enabled, active-low)
 *
 * POTENTIOMETER CONTROLS:
 *   A0: Global Brightness (0.5 - 1.0)
 *   A1: Global Decay/Sustain (multiplier on per-note decay)
 *   A2: Reverb Amount (dry/wet mix)
 *   A3: Reverb Size (room size)
 *   A4: LFO Rate (optional vibrato)
 *   A5: LFO Depth
 *
 * OLED DISPLAY (0.96" SSD1306 I2C):
 *   Pin 12 (SCL) → OLED SCL
 *   Pin 13 (SDA) → OLED SDA
 *   Shows: Active notes, tuning, parameters
 *
 * LED: Blinks when any note is triggered
 *
 * FEATURES:
 *   - Full polyphony (all 7 notes can play simultaneously)
 *   - Frequency-dependent decay times (low notes sustain longer)
 *   - Frequency-dependent brightness (high notes are brighter)
 *   - Authentic kalimba character using Karplus-Strong synthesis
 *   - Real-time control over sustain and brightness
 *   - Optional LFO modulation for expressive vibrato
 *   - Reverb for resonator box simulation
 */

#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace daisysp;

// Hardware
DaisySeed hw;

// OLED Display
OledDisplay<SSD130xI2c128x64Driver> display;

// DSP modules - 7 independent Karplus-Strong strings
const int NUM_STRINGS = 7;
String strings[NUM_STRINGS];

// Optional LFO modulation
Oscillator lfo_vibrato;
Oscillator lfo_tremolo;

// Reverb for resonator box simulation
// ReverbSc reverb;  // TODO: Re-enable when DaisySP LGPL is properly configured

// DC blocker (essential for Karplus-Strong)
DcBlock dc_blocker;

// Button GPIO pins
GPIO buttons[NUM_STRINGS];
const Pin button_pins[NUM_STRINGS] = {
    seed::D15,  // Button 1 (center)
    seed::D16,  // Button 2 (left 1)
    seed::D17,  // Button 3 (right 1)
    seed::D18,  // Button 4 (left 2)
    seed::D19,  // Button 5 (right 2)
    seed::D20,  // Button 6 (left 3)
    seed::D21   // Button 7 (right 3)
};

// Note names for display
const char* note_names[NUM_STRINGS] = {
    "G4", "A3", "B4", "D4", "E4", "G3", "A4"
};

// G Major Pentatonic tuning (traditional kalimba)
const float base_frequencies[NUM_STRINGS] = {
    392.00f,  // G4 - center
    220.00f,  // A3 - left 1
    493.88f,  // B4 - right 1 (highest)
    293.66f,  // D4 - left 2
    329.63f,  // E4 - right 2
    196.00f,  // G3 - left 3 (lowest)
    440.00f   // A4 - right 3
};

// Frequency-dependent damping for authentic kalimba decay
// Low notes sustain longer, high notes decay faster
const float base_damping[NUM_STRINGS] = {
    0.96f,  // G4 - medium-long sustain
    0.98f,  // A3 - very long sustain (low)
    0.87f,  // B4 - shorter sustain (highest note)
    0.94f,  // D4 - long sustain
    0.92f,  // E4 - medium sustain
    0.98f,  // G3 - longest sustain (lowest note)
    0.90f   // A4 - medium sustain
};

// Frequency-dependent brightness for tonal balance
// High notes brighter, low notes warmer
const float base_brightness[NUM_STRINGS] = {
    0.82f,  // G4
    0.70f,  // A3 - warmest (low)
    0.90f,  // B4 - brightest (high)
    0.76f,  // D4
    0.78f,  // E4
    0.70f,  // G3 - warmest (lowest)
    0.85f   // A4 - bright
};

// Controls
AdcChannelConfig adc_config[6];
AnalogControl controls[6];

// Control parameters
float global_brightness = 0.75f;
float global_decay = 1.0f;
float reverb_amount = 0.3f;
float reverb_size = 0.85f;
float lfo_rate = 2.0f;
float lfo_depth = 0.1f;

// Button state tracking
bool button_state[NUM_STRINGS];
bool button_triggered[NUM_STRINGS];

// LED timing
uint32_t led_timer = 0;
const uint32_t LED_ON_TIME = 4800;  // 100ms at 48kHz

// Display state
bool display_initialized = false;
bool display_available = false;
uint32_t display_update_timer = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 4800;  // ~100ms

// Track active notes for display
bool notes_active[NUM_STRINGS];
uint32_t note_activity_timer[NUM_STRINGS];
const uint32_t NOTE_DISPLAY_TIME = 48000;  // 1 second

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Update controls (once per block)
    for (int i = 0; i < 6; i++) {
        controls[i].Process();
    }

    // Read control values
    float pot_brightness   = controls[0].Value();
    float pot_decay        = controls[1].Value();
    float pot_reverb_mix   = controls[2].Value();
    float pot_reverb_size  = controls[3].Value();
    float pot_lfo_rate     = controls[4].Value();
    float pot_lfo_depth    = controls[5].Value();

    // Map controls
    global_brightness = 0.5f + (pot_brightness * 0.5f);  // 0.5 - 1.0
    global_decay = 0.5f + (pot_decay * 0.5f);            // 0.5 - 1.0 multiplier
    reverb_amount = pot_reverb_mix;                      // 0.0 - 1.0
    reverb_size = 0.7f + (pot_reverb_size * 0.25f);     // 0.7 - 0.95
    lfo_rate = 0.1f * powf(200.0f, pot_lfo_rate);       // 0.1 - 20 Hz
    lfo_depth = pot_lfo_depth * 0.15f;                   // 0 - 15% modulation

    // Update reverb parameters
    // reverb.SetFeedback(reverb_size);
    // reverb.SetLpFreq(8000.0f);

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);
    lfo_tremolo.SetFreq(lfo_rate * 0.7f);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        float output = 0.0f;

        // Process LFOs for optional modulation
        float vibrato_sig = lfo_vibrato.Process();
        float tremolo_sig = lfo_tremolo.Process();

        // Process all 7 strings (full polyphony)
        for (int s = 0; s < NUM_STRINGS; s++) {
            // Check for trigger
            bool trigger = button_triggered[s];
            if (trigger) {
                button_triggered[s] = false;  // Clear trigger flag
                notes_active[s] = true;
                note_activity_timer[s] = NOTE_DISPLAY_TIME;

                // Blink LED on any trigger
                led_timer = LED_ON_TIME;
            }

            // Update string parameters with global controls
            float adjusted_damping = base_damping[s] * global_decay;
            adjusted_damping = fclamp(adjusted_damping, 0.5f, 0.99f);
            strings[s].SetDamping(adjusted_damping);

            // Apply vibrato modulation (pitch)
            float pitch_mod = 1.0f + (vibrato_sig * 0.01f * lfo_depth);
            strings[s].SetFreq(base_frequencies[s] * pitch_mod);

            // Apply brightness with global control
            float adjusted_brightness = base_brightness[s] * global_brightness;
            adjusted_brightness = fclamp(adjusted_brightness, 0.3f, 1.0f);
            strings[s].SetBrightness(adjusted_brightness);

            // Generate string output
            float string_output = strings[s].Process(trigger);

            // Apply tremolo (amplitude modulation)
            float amp_mod = 1.0f - (fabsf(tremolo_sig) * 0.3f * lfo_depth);
            string_output *= amp_mod;

            output += string_output;
        }

        // Scale down polyphonic output to prevent clipping
        output *= 0.35f;  // 7 voices max, scale accordingly

        // Remove DC offset (critical for Karplus-Strong)
        output = dc_blocker.Process(output);

        // Process reverb (simulate resonator box) - DISABLED for now
        // float reverb_l, reverb_r;
        // reverb.Process(output, output, &reverb_l, &reverb_r);

        // Dry/wet mix - DISABLED for now
        // float dry = output * (1.0f - reverb_amount);
        // float wet = (reverb_l + reverb_r) * 0.5f * reverb_amount;
        // output = dry + wet;

        // Soft saturation for warmth
        output = tanhf(output * 1.2f) * 0.8f;

        // Output (mono to both channels)
        out[0][i] = output;
        out[1][i] = output;

        // Decrement LED timer
        if (led_timer > 0) {
            led_timer--;
        }

        // Increment display update timer
        display_update_timer++;

        // Decrement note activity timers
        for (int s = 0; s < NUM_STRINGS; s++) {
            if (note_activity_timer[s] > 0) {
                note_activity_timer[s]--;
                if (note_activity_timer[s] == 0) {
                    notes_active[s] = false;
                }
            }
        }
    }
}

void UpdateDisplay() {
    if (!display_available) return;

    // Clear display
    display.Fill(false);

    // Title
    display.SetCursor(0, 0);
    display.WriteString("DIGITAL KALIMBA", Font_6x8, true);

    // Tuning indicator
    display.SetCursor(0, 10);
    display.WriteString("G Major Pentatonic", Font_6x8, true);

    // Show active notes
    display.SetCursor(0, 22);
    display.WriteString("Playing:", Font_6x8, true);

    int active_count = 0;
    char active_notes[32] = "";
    for (int i = 0; i < NUM_STRINGS; i++) {
        if (notes_active[i]) {
            if (active_count > 0) {
                strcat(active_notes, " ");
            }
            strcat(active_notes, note_names[i]);
            active_count++;
        }
    }

    if (active_count == 0) {
        strcpy(active_notes, "---");
    }

    display.SetCursor(0, 32);
    display.WriteString(active_notes, Font_6x8, true);

    // Show controls
    char str_buf[32];
    display.SetCursor(0, 44);
    sprintf(str_buf, "Decay:%.1f Rev:%.0f%%", global_decay, reverb_amount * 100.0f);
    display.WriteString(str_buf, Font_6x8, true);

    display.SetCursor(0, 54);
    sprintf(str_buf, "Brite:%.2f LFO:%.1f", global_brightness, lfo_depth * 100.0f);
    display.WriteString(str_buf, Font_6x8, true);

    // Update display
    display.Update();
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);  // Low latency
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL: Codec stabilization delay
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

    // Initialize buttons with pull-up resistors (active-low)
    for (int i = 0; i < NUM_STRINGS; i++) {
        buttons[i].Init(button_pins[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        button_state[i] = false;
        button_triggered[i] = false;
        notes_active[i] = false;
        note_activity_timer[i] = 0;
    }

    // Initialize Karplus-Strong strings
    for (int i = 0; i < NUM_STRINGS; i++) {
        strings[i].Init(sample_rate);
        strings[i].SetFreq(base_frequencies[i]);
        strings[i].SetDamping(base_damping[i]);
        strings[i].SetBrightness(base_brightness[i]);
        strings[i].SetNonLinearity(0.1f);  // Metallic kalimba character
    }

    // Initialize LFOs
    lfo_vibrato.Init(sample_rate);
    lfo_vibrato.SetWaveform(Oscillator::WAVE_SIN);
    lfo_vibrato.SetAmp(1.0f);
    lfo_vibrato.SetFreq(5.0f);

    lfo_tremolo.Init(sample_rate);
    lfo_tremolo.SetWaveform(Oscillator::WAVE_TRI);
    lfo_tremolo.SetAmp(1.0f);
    lfo_tremolo.SetFreq(3.5f);

    // Initialize reverb
    // reverb.Init(sample_rate);
    // reverb.SetFeedback(0.85f);
    // reverb.SetLpFreq(8000.0f);

    // Initialize DC blocker
    dc_blocker.Init(sample_rate);

    // Start audio BEFORE OLED init
    hw.StartAudio(AudioCallback);

    // Allow audio to stabilize
    System::Delay(50);

    // Initialize OLED display
    display_initialized = false;
    display_available = false;

    // Main loop
    uint32_t loop_counter = 0;
    while(1) {
        // Update LED
        hw.SetLed(led_timer > 0);

        // Initialize OLED on first iteration
        if (!display_initialized) {
            OledDisplay<SSD130xI2c128x64Driver>::Config disp_cfg;

            // Try common I2C addresses
            uint8_t addresses[] = {0x3C, 0x3D};
            bool display_found = false;

            for(int addr_idx = 0; addr_idx < 2 && !display_found; addr_idx++) {
                disp_cfg.driver_config.transport_config.i2c_address = addresses[addr_idx];
                disp_cfg.driver_config.transport_config.i2c_config.periph = I2CHandle::Config::Peripheral::I2C_1;
                disp_cfg.driver_config.transport_config.i2c_config.speed = I2CHandle::Config::Speed::I2C_100KHZ;
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = seed::D11;
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = seed::D12;

                display.Init(disp_cfg);

                // Show splash screen
                display.Fill(false);
                display.SetCursor(10, 20);
                display.WriteString("DIGITAL", Font_7x10, true);
                display.SetCursor(20, 35);
                display.WriteString("KALIMBA", Font_7x10, true);
                display.Update();

                display_found = true;
                display_available = true;
            }

            display_initialized = true;
            System::Delay(1000);  // Show splash
        }

        // Button scanning with edge detection
        for (int i = 0; i < NUM_STRINGS; i++) {
            bool current = !buttons[i].Read();  // Active-low (pressed = true when LOW)

            // Rising edge detection (button just pressed)
            if (current && !button_state[i]) {
                button_triggered[i] = true;
            }

            button_state[i] = current;
        }

        // Time-sliced display updates
        if (display_update_timer >= DISPLAY_UPDATE_INTERVAL) {
            UpdateDisplay();
            display_update_timer = 0;
        }

        // Main loop delay
        System::Delay(1);
        loop_counter++;
    }
}
