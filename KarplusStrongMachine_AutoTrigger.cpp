/*
 * KARPLUS-STRONG MACHINE - AUTO TRIGGER TEST VERSION
 *
 * This version auto-triggers every 2 seconds at a fixed pitch.
 * Use this to verify audio output is working before wiring pots.
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
String str;
DcBlock dc_blocker;

uint32_t trigger_timer = 0;
const uint32_t TRIGGER_INTERVAL = 96000;  // 2 seconds at 48kHz

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    for (size_t i = 0; i < size; i++) {
        // Auto-trigger every 2 seconds
        trigger_timer++;
        bool trigger = false;
        if (trigger_timer >= TRIGGER_INTERVAL) {
            trigger = true;
            trigger_timer = 0;
        }

        // Process string (fixed at 220 Hz, A3)
        float output = str.Process(trigger);

        // Remove DC
        output = dc_blocker.Process(output);

        // Boost volume for testing
        output *= 1.5f;

        // Soft limit
        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;

        // Output (mono to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    // Initialize string with audible settings
    str.Init(sample_rate);
    str.SetFreq(220.0f);      // A3 - middle range
    str.SetDamping(0.9f);     // Long decay
    str.SetBrightness(0.7f);  // Bright (easier to hear)
    str.SetNonLinearity(0.1f);

    dc_blocker.Init(sample_rate);

    hw.StartAudio(AudioCallback);

    while(1) {
        // Blink LED to show it's running
        hw.SetLed(trigger_timer < (TRIGGER_INTERVAL / 2));
        System::Delay(1);
    }
}
