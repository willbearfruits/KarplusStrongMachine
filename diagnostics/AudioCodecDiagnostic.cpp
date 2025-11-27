/*
 * DAISY SEED AUDIO CODEC DIAGNOSTIC TEST
 *
 * This program tests the AK4556 audio codec initialization
 * and generates a simple test tone to verify audio output.
 *
 * EXPECTED BEHAVIOR:
 * - LED blinks at 2 Hz
 * - Constant 440 Hz sine wave on audio outputs
 * - If you hear audio, codec is working
 * - If LED blinks but no audio, codec initialization failed
 *
 * HARDWARE:
 * - Daisy Seed (AK4556 codec)
 * - Audio output on pins 18/19
 */

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Oscillator osc;

bool led_state = false;
uint32_t led_counter = 0;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

    // Toggle LED at 2 Hz (48000 / 48 blocks = 1000 blocks/sec, toggle every 250 blocks)
    led_counter++;
    if (led_counter >= 250) {
        led_state = !led_state;
        led_counter = 0;
    }

    // Generate simple test tone
    for (size_t i = 0; i < size; i++) {
        float sig = osc.Process();

        // Output to both channels
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

int main(void) {
    // Initialize hardware - this calls ConfigureAudio() which initializes AK4556
    hw.Init();
    hw.SetAudioBlockSize(48);  // Use default block size
    float sample_rate = hw.AudioSampleRate();

    // CRITICAL: Allow codec to stabilize after hardware reset
    // The AK4556::Init() does a hardware reset sequence (high-low-high)
    // but the codec needs additional time to stabilize after reset
    System::Delay(100);  // 100ms should be sufficient for codec startup

    // Initialize test tone oscillator
    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(440.0f);  // A440
    osc.SetAmp(0.5f);     // 50% amplitude to avoid clipping

    // Start audio - this begins SAI transmission
    hw.StartAudio(AudioCallback);

    // Additional delay after starting audio to allow DMA to stabilize
    System::Delay(50);

    // Main loop - just update LED based on audio callback state
    while(1) {
        hw.SetLed(led_state);
        System::Delay(10);
    }
}
