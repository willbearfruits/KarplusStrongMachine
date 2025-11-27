/*
 * SERIAL DIAGNOSTIC - Daisy Seed with USB Monitoring
 *
 * This will:
 * 1. Print diagnostic info to USB serial
 * 2. Output a test tone
 * 3. Show audio callback is running
 * 4. Blink LED in pattern
 *
 * Monitor with: screen /dev/ttyACM0 115200
 * or: minicom -D /dev/ttyACM0 -b 115200
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Oscillator osc;

uint32_t callback_count = 0;
uint32_t print_counter = 0;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    callback_count++;

    for (size_t i = 0; i < size; i++) {
        // Generate 440 Hz test tone
        float sig = osc.Process();

        // Output at 50% volume
        sig *= 0.5f;

        // Both channels
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // Start USB serial logging
    hw.StartLog(true);

    System::Delay(500);

    hw.PrintLine("=================================");
    hw.PrintLine("Daisy Seed Serial Diagnostic");
    hw.PrintLine("=================================");
    hw.PrintLine("");

    // Print hardware info
    char buffer[64];
    sprintf(buffer, "Sample Rate: %.0f Hz", sample_rate);
    hw.PrintLine(buffer);

    sprintf(buffer, "Block Size: %d samples", (int)hw.AudioBlockSize());
    hw.PrintLine(buffer);

    hw.PrintLine("");
    hw.PrintLine("Audio Output: Pins 18 & 19");
    hw.PrintLine("Test Tone: 440 Hz sine wave");
    hw.PrintLine("Volume: 50%");
    hw.PrintLine("");

    // Initialize oscillator
    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(440.0f);
    osc.SetAmp(1.0f);

    hw.PrintLine("Starting audio...");

    // Start audio
    hw.StartAudio(AudioCallback);

    hw.PrintLine("Audio started!");
    hw.PrintLine("");
    hw.PrintLine("Monitoring audio callback...");
    hw.PrintLine("(Callback count updates every 2 seconds)");
    hw.PrintLine("");

    // Main loop
    while(1) {
        print_counter++;

        // Print status every 2 seconds
        if (print_counter >= 2000) {
            sprintf(buffer, "Audio callbacks: %lu", callback_count);
            hw.PrintLine(buffer);
            print_counter = 0;
        }

        // Blink LED: 2 fast, 1 slow
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);

        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(500);

        hw.SetLed(true);
        System::Delay(300);
        hw.SetLed(false);
        System::Delay(300);
    }
}
