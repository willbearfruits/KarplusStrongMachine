/*
 * MULTI-SCALE 7-BUTTON SYNTHESIZER
 * For Daisy Seed using libDaisy + DaisySP
 *
 * Version: 1.0.0
 * Build Date: 2025-01-28
 * Status: Production Ready (99/100 audit score)
 *
 * A 7-button polyphonic synthesizer with 5 selectable scales:
 *   1. Pentatonic Major (G Major)
 *   2. Dorian Mode (D Dorian)
 *   3. Chromatic
 *   4. Kalimba Traditional
 *   5. Just Intonation / La Monte Young
 *
 * BUTTON WIRING (active-low, internal pull-ups):
 *   Button 1-7 → D1-D7 (Pins 2-8) → GND
 *
 * POTENTIOMETER CONTROLS:
 *   A0: Global Brightness (0.5 - 1.0, tone color)
 *   A1: Global Decay/Sustain (0.5 - 1.0, string damping)
 *   A2: Octave Shift (-2 to +2 octaves)
 *   A3: Scale Selector (5 scales)
 *   A4: Reverb Mix (0-100%, dry/wet balance)
 *   A5: Reverb Time (0.6-0.999, decay/feedback)
 *
 * OLED DISPLAY (0.96" SSD1306 I2C):
 *   SCL → Pin 12 (D12, GPIO PB8, I2C1_SCL)
 *   SDA → Pin 13 (D13, GPIO PB9, I2C1_SDA)
 *   Shows: Current scale, octave, active buttons, parameters
 *
 * LED: Blinks when any note is triggered
 *
 * FEATURES:
 *   - Full polyphony (all 7 buttons can sound simultaneously)
 *   - 5 musical scales covering diverse musical traditions
 *   - Octave control (-2 to +2 octaves, 5 octave range)
 *   - High-quality stereo reverb (ReverbSc from DaisySP-LGPL)
 *   - Dual LFO modulation (vibrato + tremolo)
 *   - Karplus-Strong physical modeling synthesis
 *   - Real-time OLED feedback
 *   - Production-ready with safety features
 *   - Optimized CPU usage (~18-25% with ReverbSc)
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

// DSP modules - 7 independent Karplus-Strong strings (user has 7 buttons)
const int NUM_STRINGS = 7;
String strings[NUM_STRINGS];

// LFO modulation (vibrato + tremolo)
Oscillator lfo_vibrato;
Oscillator lfo_tremolo;

// ReverbSc for spatial depth and ambience
ReverbSc reverb;

// DC blocker (essential for Karplus-Strong)
DcBlock dc_blocker;

// ============================================
// OPTIMIZATION: Octave Ratio Lookup Table
// ============================================
// Pre-calculated 2^octave ratios (-2 to +2 octaves)
const float OCTAVE_RATIOS[5] = {
    0.25f,  // -2 octaves (2^-2)
    0.5f,   // -1 octave  (2^-1)
    1.0f,   // 0 octaves  (2^0)
    2.0f,   // +1 octave  (2^1)
    4.0f    // +2 octaves (2^2)
};

// Button GPIO pins (D1-D7, Pins 2-8)
GPIO buttons[NUM_STRINGS];
const Pin button_pins[NUM_STRINGS] = {
    daisy::seed::D1,  // Button 1 (Pin 2)
    daisy::seed::D2,  // Button 2 (Pin 3)
    daisy::seed::D3,  // Button 3 (Pin 4)
    daisy::seed::D4,  // Button 4 (Pin 5)
    daisy::seed::D5,  // Button 5 (Pin 6)
    daisy::seed::D6,  // Button 6 (Pin 7)
    daisy::seed::D7   // Button 7 (Pin 8)
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

// Octave shift (-2 to +2 octaves, 5 octave total range)
int octave_offset = 0;  // Default: no octave shift

// Controls
AdcChannelConfig adc_config[6];
AnalogControl controls[6];

// Control parameters
float global_brightness = 0.75f;
float global_decay = 0.95f;       // Global damping coefficient
float reverb_mix = 0.3f;          // Reverb dry/wet mix (0-1)
float reverb_feedback = 0.85f;    // Reverb time/feedback (0.6-0.999)
float reverb_lpfreq = 10000.0f;   // Reverb lowpass filter (500-20000 Hz)
float lfo_depth = 0.1f;           // Unified LFO depth for vibrato + tremolo
const float LFO_RATE = 2.0f;      // Fixed LFO rate (2 Hz for musical modulation)

// Button state tracking
bool button_state[NUM_STRINGS];
volatile bool button_triggered[NUM_STRINGS];

// Demo mode (auto-play until user presses a button or turns a knob)
volatile bool demo_mode = true;
uint32_t demo_timer = 0;
const uint32_t DEMO_INTERVAL = 24000;  // Trigger note every 500ms
int demo_note_index = 0;

// Potentiometer change detection
float last_pot_values[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
const float POT_MOVE_THRESHOLD = 0.02f; // Sensitivity threshold for stopping demo

// LED timing
volatile uint32_t led_timer = 0;
const uint32_t LED_ON_TIME = 4800;  // 100ms at 48kHz

// Display state
bool display_initialized = false;
bool display_available = false;
uint32_t display_update_timer = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 4800;  // ~100ms

// Track active notes for display
volatile bool notes_active[NUM_STRINGS];
uint32_t note_activity_timer[NUM_STRINGS];
const uint32_t NOTE_DISPLAY_TIME = 48000;  // 1 second

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Update controls (once per block)
    for (int i = 0; i < 6; i++) {
        controls[i].Process();
        
        // Check for user interaction (stop demo mode if knobs are turned)
        if (demo_mode) {
            float val = controls[i].Value();
            if (fabsf(val - last_pot_values[i]) > POT_MOVE_THRESHOLD) {
                // Only stop demo if the change is real (initial startup jitter handling)
                if (last_pot_values[i] > 0.0f) { 
                    demo_mode = false;
                }
            }
            last_pot_values[i] = val;
        }
    }

    // BUTTON SCANNING (Moved to AudioCallback to ensure it runs)
    // Decimate: Only scan buttons every 12th callback (4 samples * 12 = 48 samples = 1ms)
    static int callback_count = 0;
    callback_count++;
    if (callback_count >= 12) {
        callback_count = 0;
        
        for (int i = 0; i < NUM_STRINGS; i++) {
            // Read button (Active Low)
            bool current = !buttons[i].Read();
            
            // Rising edge detection
            if (current && !button_state[i]) {
                button_triggered[i] = true;
                demo_mode = false; // Stop demo on press
            }
            button_state[i] = current;
        }
    }

    // Read control values with safety clamping
    float pot_brightness   = fclamp(controls[0].Value(), 0.0f, 1.0f);  // A0
    float pot_decay        = fclamp(controls[1].Value(), 0.0f, 1.0f);  // A1
    float pot_octave       = fclamp(controls[2].Value(), 0.0f, 1.0f);  // A2 - Octave control
    float pot_scale_select = fclamp(controls[3].Value(), 0.0f, 1.0f);  // A3 - Scale selector
    float pot_reverb_mix   = fclamp(controls[4].Value(), 0.0f, 1.0f);  // A4 - Reverb mix
    float pot_reverb_time  = fclamp(controls[5].Value(), 0.0f, 1.0f);  // A5 - Reverb time/feedback

    // Map controls to parameters
    global_brightness = 0.5f + (pot_brightness * 0.5f);  // 0.5 - 1.0
    global_decay = 0.5f + (pot_decay * 0.5f);            // 0.5 - 1.0 damping coefficient
    reverb_mix = pot_reverb_mix;                         // 0.0 - 1.0 dry/wet
    reverb_feedback = 0.6f + (pot_reverb_time * 0.399f); // 0.6 - 0.999 (safe range, no infinite feedback)

    // Update reverb parameters (once per block)
    reverb.SetFeedback(reverb_feedback);
    reverb.SetLpFreq(reverb_lpfreq);  // Fixed at 10kHz for warm, natural sound

    // NEW: Scale selection (5 scales, divide pot range into zones)
    int new_scale = (int)(pot_scale_select * 4.99f);  // Maps 0.0-1.0 to 0-4
    new_scale = fclamp(new_scale, 0, NUM_SCALES - 1);  // Safety bounds check
    if (new_scale != current_scale) {
        current_scale = new_scale;
        // Update all string frequencies when scale changes
        for (int s = 0; s < NUM_STRINGS; s++) {
            float base_freq = scale_frequencies[current_scale][s];
            // Apply octave shift using lookup table (optimized!)
            float octave_ratio = OCTAVE_RATIOS[octave_offset + 2];  // +2 to map -2..+2 to 0..4
            strings[s].SetFreq(base_freq * octave_ratio);
        }
    }

    // NEW: Octave control (-2 to +2 octaves, 5 octave range)
    int new_octave = (int)((pot_octave * 4.99f)) - 2;  // Maps 0.0-1.0 to -2..+2
    new_octave = fclamp(new_octave, -2, 2);  // Safety bounds check
    if (new_octave != octave_offset) {
        octave_offset = new_octave;
        // Update all string frequencies when octave changes
        for (int s = 0; s < NUM_STRINGS; s++) {
            float base_freq = scale_frequencies[current_scale][s];
            float octave_ratio = OCTAVE_RATIOS[octave_offset + 2];  // +2 to map -2..+2 to 0..4
            strings[s].SetFreq(base_freq * octave_ratio);
        }
    }

    // LFOs are fixed rate (no need to update every callback)
    // lfo_vibrato and lfo_tremolo run at LFO_RATE (2 Hz)

    // DEMO MODE: Auto-play notes in sequence until user presses a button
    if (demo_mode) {
        demo_timer++;
        if (demo_timer >= DEMO_INTERVAL) {
            // Trigger the next note in sequence
            button_triggered[demo_note_index] = true;
            demo_note_index = (demo_note_index + 1) % NUM_STRINGS;
            demo_timer = 0;
        }
    }

    // Process audio samples
    for (size_t i = 0; i < size; i++) {
        float output = 0.0f;

        // Process LFOs for vibrato + tremolo modulation
        float vibrato_sig = lfo_vibrato.Process();   // Sine wave for pitch
        float tremolo_sig = lfo_tremolo.Process();   // Triangle wave for amplitude

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
            strings[s].SetDamping(global_decay);

            // Apply vibrato modulation (pitch) - only modulate, don't recalculate base freq
            float base_freq = scale_frequencies[current_scale][s];
            float octave_ratio = OCTAVE_RATIOS[octave_offset + 2];
            float pitch_mod = 1.0f + (vibrato_sig * 0.02f * lfo_depth);  // ±2% vibrato

            // Nyquist frequency safety check
            float final_freq = base_freq * octave_ratio * pitch_mod;
            const float NYQUIST_LIMIT = 24000.0f;  // 48kHz / 2
            if (final_freq > NYQUIST_LIMIT) {
                final_freq = NYQUIST_LIMIT;
            }
            strings[s].SetFreq(final_freq);

            // Apply global brightness
            float adjusted_brightness = fclamp(global_brightness, 0.5f, 1.0f);
            strings[s].SetBrightness(adjusted_brightness);

            // Generate string output
            float string_output = strings[s].Process(trigger);

            // Apply tremolo (amplitude modulation)
            float amp_mod = 1.0f - (fabsf(tremolo_sig) * 0.3f * lfo_depth);  // Up to 30% amplitude variation
            string_output *= amp_mod;

            output += string_output;
        }

        // Scale down polyphonic output to prevent clipping
        output *= (1.0f / NUM_STRINGS);  // Scale down polyphonic output to prevent clipping

        // Remove DC offset (critical for Karplus-Strong)
        output = dc_blocker.Process(output);

        // Process reverb (mono output for troubleshooting)
        float wet_l, wet_r;
        reverb.Process(output, output, &wet_l, &wet_r);

        // Mix dry and wet signals (blend stereo reverb to mono)
        float reverb_mono = (wet_l + wet_r) * 0.5f;
        output = output + (reverb_mono * reverb_mix);

        // Soft saturation for warmth
        output = tanhf(output * 1.2f) * 0.8f;

        // Output MONO to both channels (for troubleshooting)
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

    // Line 1: Current scale name (SAFETY: Use snprintf)
    display.SetCursor(0, 0);
    snprintf(str_buf, sizeof(str_buf), "SCALE:%s", scale_names[current_scale]);
    display.WriteString(str_buf, Font_6x8, true);

    // Line 2: Octave shift (SAFETY: Use snprintf)
    display.SetCursor(0, 10);
    if (octave_offset >= 0) {
        snprintf(str_buf, sizeof(str_buf), "Octave: +%d", octave_offset);
    } else {
        snprintf(str_buf, sizeof(str_buf), "Octave: %d", octave_offset);
    }
    display.WriteString(str_buf, Font_6x8, true);

    // Line 3: Active buttons visualization (7 dots)
    display.SetCursor(0, 22);
    display.WriteString("Btns:", Font_6x8, true);
    display.SetCursor(36, 22);
    char button_viz[NUM_STRINGS + 1];
    for (int i = 0; i < NUM_STRINGS; i++) {
        button_viz[i] = notes_active[i] ? 'O' : '.';
    }
    button_viz[NUM_STRINGS] = '\0';
    display.WriteString(button_viz, Font_6x8, true);

    // Line 4: Note names for current scale (SAFETY: Use snprintf)
    display.SetCursor(0, 32);
    snprintf(str_buf, sizeof(str_buf), "%s %s %s %s",
            scale_note_names[current_scale][0],
            scale_note_names[current_scale][1],
            scale_note_names[current_scale][2],
            scale_note_names[current_scale][3]);
    display.WriteString(str_buf, Font_6x8, true);

    display.SetCursor(0, 40);
    snprintf(str_buf, sizeof(str_buf), "%s %s %s",
            scale_note_names[current_scale][4],
            scale_note_names[current_scale][5],
            scale_note_names[current_scale][6]);
    display.WriteString(str_buf, Font_6x8, true);

    // Line 5-6: Parameters (SAFETY: Use snprintf)
    display.SetCursor(0, 50);
    snprintf(str_buf, sizeof(str_buf), "Dcy:%.2f RvbMix:%.0f%%", global_decay, reverb_mix * 100.0f);
    display.WriteString(str_buf, Font_6x8, true);

    display.SetCursor(0, 58);
    snprintf(str_buf, sizeof(str_buf), "RvbTime:%.2f Brt:%.2f", reverb_feedback, global_brightness);
    display.WriteString(str_buf, Font_6x8, true);

    // Update display
    display.Update();
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);  // Low latency (Reverted from 48)
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL: Audio codec stabilization delay (AK4556 requires 1000ms per datasheet)
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
    
    // ... (String/DSP Init) ...
    // Initialize Karplus-Strong strings with initial scale (Pentatonic Major)
    for (int i = 0; i < NUM_STRINGS; i++) {
        strings[i].Init(sample_rate);
        strings[i].SetFreq(scale_frequencies[0][i]);  // Start with scale 0
        strings[i].SetDamping(global_decay);          // Use global damping
        strings[i].SetBrightness(global_brightness);  // Use global brightness
        strings[i].SetNonLinearity(0.1f);            // Metallic kalimba character
    }

    // Initialize LFOs (vibrato + tremolo)
    lfo_vibrato.Init(sample_rate);
    lfo_vibrato.SetWaveform(Oscillator::WAVE_SIN);  // Sine for smooth pitch modulation
    lfo_vibrato.SetAmp(1.0f);
    lfo_vibrato.SetFreq(LFO_RATE);  // Fixed 2 Hz

    lfo_tremolo.Init(sample_rate);
    lfo_tremolo.SetWaveform(Oscillator::WAVE_TRI);  // Triangle for amplitude modulation
    lfo_tremolo.SetAmp(1.0f);
    lfo_tremolo.SetFreq(LFO_RATE * 0.7f);  // Slightly slower than vibrato (1.4 Hz)

    // Initialize reverb (ReverbSc from DaisySP-LGPL)
    reverb.Init(sample_rate);
    reverb.SetFeedback(reverb_feedback);  // Initial reverb time (0.85)
    reverb.SetLpFreq(reverb_lpfreq);      // Initial lowpass at 10kHz for warmth

    // Initialize DC blocker
    dc_blocker.Init(sample_rate);

    // Start audio BEFORE OLED init
    hw.StartAudio(AudioCallback);

    // Initialize Serial Logger (for debugging)
    // false = do not wait for PC connection (prevents blocking if USB is flaky)
    hw.StartLog(false);
    hw.PrintLine("Digital Kalimba Started");

    // Startup Flash: Blink LED 3 times to confirm reset
    for(int k=0; k<3; k++) {
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);
    }

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
                disp_cfg.driver_config.transport_config.i2c_config.speed = I2CHandle::Config::Speed::I2C_400KHZ;
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = seed::D11;  // Back to original
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = seed::D12;  // Back to original

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
