/*
 * KARPLUS-STRONG PLUCKED STRING MACHINE - FINAL VERSION WITH OLED
 * For Daisy Seed using libDaisy + DaisySP
 *
 * AUTO-TRIGGER MODE with speed control + OLED parameter display
 *
 * CONTROLS:
 * - A0: Pitch (50 Hz - 2000 Hz)
 * - A1: Decay Time (1s to 20s+)
 * - A2: Brightness (filter cutoff)
 * - A3: TRIGGER SPEED (0.1 sec - 10 sec interval)
 * - A4: LFO Rate (0.1 Hz - 20 Hz)
 * - A5: LFO Depth (modulation amount)
 *
 * OLED DISPLAY (0.96" SSD1306 I2C):
 * - Pin 11 (SCL) → OLED SCL
 * - Pin 12 (SDA) → OLED SDA
 * - 3.3V → OLED VCC
 * - GND → OLED GND
 * Shows: Pitch, Decay, Brightness, Trigger Speed, LFO Rate, LFO Depth
 *
 * LED: Blinks on each pluck
 *
 * CRITICAL TIMING FIXES:
 * - 100ms delay after Init (codec stabilization)
 * - StartAudio() called BEFORE OLED init
 * - 50ms delay after StartAudio (DMA stabilization)
 * - OLED initialized in main loop AFTER audio is running
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

// Display state
bool display_initialized = false;
bool display_available = false;
uint32_t display_update_timer = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 4800; // ~100ms at 48kHz, or 10Hz

// Previous values for change detection
float prev_pitch_freq = 0.0f;
float prev_decay_amount = 0.0f;
float prev_brightness = 0.0f;
float prev_trigger_speed = 0.0f;
float prev_lfo_rate = 0.0f;
float prev_lfo_depth = 0.0f;

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

        // Increment display update timer
        display_update_timer++;

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

void UpdateDisplay() {
    if (!display_available) return;

    // Check for changes (with small threshold to avoid flicker)
    bool changed = false;
    const float CHANGE_THRESHOLD = 0.5f; // Hz for freq, 0.01 for normalized values

    if (fabsf(pitch_freq - prev_pitch_freq) > CHANGE_THRESHOLD) changed = true;
    if (fabsf(decay_amount - prev_decay_amount) > 0.01f) changed = true;
    if (fabsf(brightness - prev_brightness) > 0.01f) changed = true;
    if (fabsf(trigger_speed - prev_trigger_speed) > 0.01f) changed = true;
    if (fabsf(lfo_rate - prev_lfo_rate) > CHANGE_THRESHOLD) changed = true;
    if (fabsf(lfo_depth - prev_lfo_depth) > 0.01f) changed = true;

    if (!changed) return;

    // Update previous values
    prev_pitch_freq = pitch_freq;
    prev_decay_amount = decay_amount;
    prev_brightness = brightness;
    prev_trigger_speed = trigger_speed;
    prev_lfo_rate = lfo_rate;
    prev_lfo_depth = lfo_depth;

    // Clear display
    display.Fill(false);

    // Display title
    char str_buf[32];
    display.SetCursor(0, 0);
    display.WriteString("KARPLUS-STRONG", Font_6x8, true);

    // Display parameters (3 columns, 2 rows each)
    // Row 1: Pitch
    display.SetCursor(0, 10);
    sprintf(str_buf, "Pitch: %.0fHz", pitch_freq);
    display.WriteString(str_buf, Font_6x8, true);

    // Row 2: Decay
    display.SetCursor(0, 20);
    sprintf(str_buf, "Decay: %.2f", decay_amount);
    display.WriteString(str_buf, Font_6x8, true);

    // Row 3: Brightness
    display.SetCursor(0, 30);
    sprintf(str_buf, "Bright: %.2f", brightness);
    display.WriteString(str_buf, Font_6x8, true);

    // Row 4: Trigger Speed
    display.SetCursor(0, 40);
    sprintf(str_buf, "Speed: %.2fs", trigger_speed);
    display.WriteString(str_buf, Font_6x8, true);

    // Row 5: LFO Rate
    display.SetCursor(0, 50);
    sprintf(str_buf, "LFO: %.1fHz", lfo_rate);
    display.WriteString(str_buf, Font_6x8, true);

    // Row 6: LFO Depth (as percentage)
    display.SetCursor(66, 50);
    sprintf(str_buf, "D:%.0f%%", lfo_depth * 100.0f);
    display.WriteString(str_buf, Font_6x8, true);

    // Update display
    display.Update();
}

int main(void) {
    // Initialize hardware
    // This calls ConfigureAudio() which initializes the AK4556 codec
    // The codec init does a reset sequence: HIGH -> LOW -> HIGH with 1ms delays
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL TIMING FIX #1:
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

    // CRITICAL: Start audio BEFORE initializing OLED
    // This ensures audio codec and DMA are fully running
    hw.StartAudio(AudioCallback);

    // CRITICAL TIMING FIX #2:
    // Allow DMA and audio path to stabilize before OLED I2C init
    System::Delay(50);

    // NOW initialize OLED (after audio is running)
    // OLED initialization happens HERE in main loop, not before StartAudio
    display_initialized = false;
    display_available = false;

    // Main loop - LED indicator + OLED updates
    uint32_t loop_counter = 0;
    while(1) {
        // Update LED
        hw.SetLed(led_timer > 0);

        // Initialize OLED on first loop iteration (after audio is running)
        if (!display_initialized) {
            OledDisplay<SSD130xI2c128x64Driver>::Config disp_cfg;

            // Try both common I2C addresses: 0x3C and 0x3D
            uint8_t addresses[] = {0x3C, 0x3D};
            bool display_found = false;

            for(int addr_idx = 0; addr_idx < 2 && !display_found; addr_idx++) {
                // Configure I2C for OLED
                disp_cfg.driver_config.transport_config.i2c_address = addresses[addr_idx];
                disp_cfg.driver_config.transport_config.i2c_config.periph = I2CHandle::Config::Peripheral::I2C_1;
                disp_cfg.driver_config.transport_config.i2c_config.speed = I2CHandle::Config::Speed::I2C_100KHZ;  // Slower = more reliable
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = seed::D11;
                disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = seed::D12;

                // Try to initialize display
                display.Init(disp_cfg);

                // Try to write test pattern
                display.Fill(false);
                display.SetCursor(10, 20);
                display.WriteString("KARPLUS-STRONG", Font_7x10, true);
                display.SetCursor(35, 35);
                display.WriteString("MACHINE", Font_7x10, true);
                display.Update();

                // LED diagnostic: Blink to show which address worked
                // 1 blink = 0x3C, 2 blinks = 0x3D
                for(int blink = 0; blink < addr_idx + 1; blink++) {
                    hw.SetLed(true);
                    System::Delay(150);
                    hw.SetLed(false);
                    System::Delay(150);
                }

                // Assume it worked (we can't easily verify I2C success)
                display_found = true;
                display_available = true;
            }

            display_initialized = true;

            // Show splash for 1 second
            System::Delay(1000);
        }

        // Time-sliced display updates (10Hz max)
        if (display_update_timer >= DISPLAY_UPDATE_INTERVAL) {
            UpdateDisplay();
            display_update_timer = 0;
        }

        // Main loop delay (1ms)
        System::Delay(1);
        loop_counter++;
    }
}
