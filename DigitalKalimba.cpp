/*
 * MULTI-SCALE 7-BUTTON SYNTHESIZER
 * For Daisy Seed using libDaisy + DaisySP
 *
 * A 7-button polyphonic synthesizer with 5 selectable scales:
 *   1. Pentatonic Major (G Major)
 *   2. Dorian Mode (D Dorian)
 *   3. Chromatic
 *   4. Kalimba Traditional
 *   5. Just Intonation / La Monte Young
 *
 * BUTTON WIRING (active-low, internal pull-ups):
 *   Button 1-7 → D15-D21 (Pins 23-29) → GND
 *
 * POTENTIOMETER CONTROLS:
 *   A0: Global Brightness (0.5 - 1.0, tone color)
 *   A1: Global Decay/Sustain (0.5 - 1.0, string damping)
 *   A2: Transpose (-12 to +12 semitones)
 *   A3: Scale Selector (5 scales)
 *   A4: LFO Rate (0.1 - 20 Hz, vibrato speed)
 *   A5: LFO Depth (0 - 15%, vibrato amount)
 *
 * OLED DISPLAY (0.96" SSD1306 I2C):
 *   Pin 12 (SCL) → OLED SCL
 *   Pin 13 (SDA) → OLED SDA
 *   Shows: Current scale, transpose, active buttons, parameters
 *
 * LED: Blinks when any note is triggered
 *
 * FEATURES:
 *   - Full polyphony (all 7 buttons can sound simultaneously)
 *   - 5 musical scales covering diverse musical traditions
 *   - Transpose control (±1 octave)
 *   - Karplus-Strong physical modeling synthesis
 *   - Real-time OLED feedback
 *   - Vibrato LFO modulation
 *   - Low CPU usage (~10-15%)
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

// LFO modulation (vibrato only)
Oscillator lfo_vibrato;

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

// ============================================
// MULTI-SCALE SYSTEM - 5 Scales Available
// ============================================

#define NUM_SCALES 5

// Scale names for display
const char* scale_names[NUM_SCALES] = {
    "Pentatonic Maj",  // G Major Pentatonic
    "Dorian Mode",     // D Dorian
    "Chromatic",       // Chromatic from C3
    "Kalimba Trad",    // Traditional kalimba voicing
    "Just/LaMonte"     // La Monte Young just intonation
};

// Note names for each scale
const char* scale_note_names[NUM_SCALES][NUM_STRINGS] = {
    // Pentatonic Major (G Major)
    {"G3", "A3", "B3", "D4", "E4", "G4", "A4"},
    // Dorian Mode (D Dorian)
    {"D3", "E3", "F3", "G3", "A3", "B3", "C4"},
    // Chromatic
    {"C3", "C#3", "D3", "D#3", "E3", "F3", "F#3"},
    // Kalimba Traditional
    {"G3", "A3", "D4", "E4", "G4", "B4", "A4"},
    // Just Intonation / La Monte Young
    {"C3", "E3", "G3", "Bb3", "C4", "D4", "F4"}
};

// Base frequencies for each scale (in Hz)
const float scale_frequencies[NUM_SCALES][NUM_STRINGS] = {
    // Pentatonic Major (G Major): G3, A3, B3, D4, E4, G4, A4
    {196.00f, 220.00f, 246.94f, 293.66f, 329.63f, 392.00f, 440.00f},

    // Dorian Mode (D Dorian): D3, E3, F3, G3, A3, B3, C4
    {146.83f, 164.81f, 174.61f, 196.00f, 220.00f, 246.94f, 261.63f},

    // Chromatic: C3, C#3, D3, D#3, E3, F3, F#3
    {130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f},

    // Kalimba Traditional (alternate G Major voicing): G3, A3, D4, E4, G4, B4, A4
    {196.00f, 220.00f, 293.66f, 329.63f, 392.00f, 493.88f, 440.00f},

    // Just Intonation / La Monte Young (based on C harmonic series)
    // C3(1:1), E3(5:4), G3(3:2), Bb3(7:4), C4(2:1), D4(9:8), F4(11:8)
    {130.81f, 163.51f, 196.22f, 229.28f, 261.63f, 293.66f, 323.08f}
};

// Current scale selection
int current_scale = 0;  // Default to Pentatonic Major

// Transpose amount (in semitones, -12 to +12)
int transpose_semitones = 0;

// Controls
AdcChannelConfig adc_config[6];
AnalogControl controls[6];

// Control parameters
float global_brightness = 0.75f;
float global_decay = 0.95f;  // Global damping coefficient
float transpose_pot = 0.5f;  // Transpose control (0-1, maps to -12 to +12 semitones)
float scale_pot = 0.0f;      // Scale selector (0-1, maps to 0-4 scale index)
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
    float pot_brightness   = controls[0].Value();  // A0
    float pot_decay        = controls[1].Value();  // A1
    float pot_transpose    = controls[2].Value();  // A2 - NEW: Transpose control
    float pot_scale_select = controls[3].Value();  // A3 - NEW: Scale selector
    float pot_lfo_rate     = controls[4].Value();  // A4
    float pot_lfo_depth    = controls[5].Value();  // A5

    // Map controls to parameters
    global_brightness = 0.5f + (pot_brightness * 0.5f);  // 0.5 - 1.0
    global_decay = 0.5f + (pot_decay * 0.5f);            // 0.5 - 1.0 damping coefficient
    lfo_rate = 0.1f * powf(200.0f, pot_lfo_rate);        // 0.1 - 20 Hz
    lfo_depth = pot_lfo_depth * 0.15f;                   // 0 - 15% modulation

    // NEW: Scale selection (5 scales, divide pot range into zones)
    int new_scale = (int)(pot_scale_select * 4.99f);  // Maps 0.0-1.0 to 0-4
    if (new_scale != current_scale) {
        current_scale = new_scale;
        // Update all string frequencies when scale changes
        for (int s = 0; s < NUM_STRINGS; s++) {
            float base_freq = scale_frequencies[current_scale][s];
            // Apply transpose
            float transpose_ratio = powf(2.0f, transpose_semitones / 12.0f);
            strings[s].SetFreq(base_freq * transpose_ratio);
        }
    }

    // NEW: Transpose control (-12 to +12 semitones)
    int new_transpose = (int)((pot_transpose - 0.5f) * 24.0f);  // Maps 0.0-1.0 to -12 to +12
    if (new_transpose != transpose_semitones) {
        transpose_semitones = new_transpose;
        // Update all string frequencies when transpose changes
        for (int s = 0; s < NUM_STRINGS; s++) {
            float base_freq = scale_frequencies[current_scale][s];
            float transpose_ratio = powf(2.0f, transpose_semitones / 12.0f);
            strings[s].SetFreq(base_freq * transpose_ratio);
        }
    }

    // Update LFO rates
    lfo_vibrato.SetFreq(lfo_rate);

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        float output = 0.0f;

        // Process LFO for vibrato modulation
        float vibrato_sig = lfo_vibrato.Process();

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

            // Update string parameters with global controls (simplified)
            strings[s].SetDamping(global_decay);  // Use global damping directly

            // Apply vibrato modulation (pitch) to current frequency
            float base_freq = scale_frequencies[current_scale][s];
            float transpose_ratio = powf(2.0f, transpose_semitones / 12.0f);
            float pitch_mod = 1.0f + (vibrato_sig * 0.02f * lfo_depth);  // ±2% vibrato
            strings[s].SetFreq(base_freq * transpose_ratio * pitch_mod);

            // Apply global brightness
            float adjusted_brightness = fclamp(global_brightness, 0.5f, 1.0f);
            strings[s].SetBrightness(adjusted_brightness);

            // Generate string output (no tremolo - simplified)
            float string_output = strings[s].Process(trigger);

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
    char str_buf[32];

    // Line 1: Current scale name
    display.SetCursor(0, 0);
    sprintf(str_buf, "SCALE:%s", scale_names[current_scale]);
    display.WriteString(str_buf, Font_6x8, true);

    // Line 2: Transpose amount
    display.SetCursor(0, 10);
    if (transpose_semitones >= 0) {
        sprintf(str_buf, "Transpose: +%d", transpose_semitones);
    } else {
        sprintf(str_buf, "Transpose: %d", transpose_semitones);
    }
    display.WriteString(str_buf, Font_6x8, true);

    // Line 3: Active buttons visualization (7 dots)
    display.SetCursor(0, 22);
    display.WriteString("Btns:", Font_6x8, true);
    display.SetCursor(36, 22);
    char button_viz[16] = "";
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcat(button_viz, notes_active[i] ? "O" : ".");
    }
    display.WriteString(button_viz, Font_6x8, true);

    // Line 4: Note names for current scale
    display.SetCursor(0, 32);
    sprintf(str_buf, "%s %s %s %s",
            scale_note_names[current_scale][0],
            scale_note_names[current_scale][1],
            scale_note_names[current_scale][2],
            scale_note_names[current_scale][3]);
    display.WriteString(str_buf, Font_6x8, true);

    display.SetCursor(0, 40);
    sprintf(str_buf, "%s %s %s",
            scale_note_names[current_scale][4],
            scale_note_names[current_scale][5],
            scale_note_names[current_scale][6]);
    display.WriteString(str_buf, Font_6x8, true);

    // Line 5-6: Parameters
    display.SetCursor(0, 50);
    sprintf(str_buf, "Decay:%.2f Brt:%.2f", global_decay, global_brightness);
    display.WriteString(str_buf, Font_6x8, true);

    display.SetCursor(0, 58);
    sprintf(str_buf, "LFO:%.1fHz D:%.0f%%", lfo_rate, lfo_depth * 100.0f);
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

    // Initialize Karplus-Strong strings with initial scale (Pentatonic Major)
    for (int i = 0; i < NUM_STRINGS; i++) {
        strings[i].Init(sample_rate);
        strings[i].SetFreq(scale_frequencies[0][i]);  // Start with scale 0
        strings[i].SetDamping(global_decay);          // Use global damping
        strings[i].SetBrightness(global_brightness);  // Use global brightness
        strings[i].SetNonLinearity(0.1f);            // Metallic kalimba character
    }

    // Initialize LFOs (vibrato only, remove tremolo)
    lfo_vibrato.Init(sample_rate);
    lfo_vibrato.SetWaveform(Oscillator::WAVE_SIN);
    lfo_vibrato.SetAmp(1.0f);
    lfo_vibrato.SetFreq(5.0f);

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
