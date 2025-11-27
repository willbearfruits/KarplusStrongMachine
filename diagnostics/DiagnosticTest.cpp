/*
 * DIAGNOSTIC TEST - Daisy Seed Hardware Verification
 *
 * Tests:
 * 1. LED blinks in specific pattern (2 fast, 1 slow)
 * 2. Outputs 440 Hz tone on audio codec
 * 3. Alternates between left and right channels every 2 seconds
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Oscillator osc;

uint32_t led_counter = 0;
uint32_t channel_switch_counter = 0;
bool use_left = true;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    channel_switch_counter++;

    // Switch channels every 2 seconds (48000 samples/sec ÷ 4 samples/block × 2 sec = 24000)
    if (channel_switch_counter >= 24000) {
        use_left = !use_left;
        channel_switch_counter = 0;
    }

    for (size_t i = 0; i < size; i++) {
        float sig = osc.Process() * 0.5f;  // 440 Hz at 50% volume

        // Alternate between left and right every 2 seconds
        if (use_left) {
            out[0][i] = sig;   // Left channel
            out[1][i] = 0.0f;  // Right channel off
        } else {
            out[0][i] = 0.0f;  // Left channel off
            out[1][i] = sig;   // Right channel
        }
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // Initialize oscillator - 440 Hz
    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(440.0f);
    osc.SetAmp(1.0f);

    // Start audio
    hw.StartAudio(AudioCallback);

    // LED pattern: blink 2 fast, then 1 slow to show code is running
    while(1) {
        // Fast blink 1
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);

        // Fast blink 2
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(500);

        // Slow blink
        hw.SetLed(true);
        System::Delay(500);
        hw.SetLed(false);
        System::Delay(500);
    }
}
