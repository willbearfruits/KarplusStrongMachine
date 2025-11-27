/*
 * SIMPLE TEST TONE - Daisy Seed Audio Output Test
 *
 * Generates a 440 Hz sine wave on both audio outputs
 * No controls needed - just audio outputs
 *
 * WIRING:
 * Pin 22 (Audio Out L) -> Left channel of audio jack
 * Pin 23 (Audio Out R) -> Right channel of audio jack
 * GND -> Audio ground
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Oscillator osc;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Generate 440 Hz sine wave
        float sig = osc.Process();

        // Reduce volume to safe level (0.3 amplitude)
        sig *= 0.3f;

        // Output to both channels
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // Initialize oscillator
    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(440.0f);  // A4 note
    osc.SetAmp(1.0f);

    // Start audio
    hw.StartAudio(AudioCallback);

    // Blink LED to show it's running
    while(1) {
        hw.SetLed(true);
        System::Delay(500);
        hw.SetLed(false);
        System::Delay(500);
    }
}
