#!/bin/bash
# Build and upload audio codec diagnostic test

cd /home/glitches/Documents/KarplusStrongMachine

echo "================================"
echo "AUDIO CODEC DIAGNOSTIC TEST"
echo "================================"
echo ""
echo "This will:"
echo "1. Build a simple test tone generator (440 Hz)"
echo "2. Upload to Daisy Seed"
echo "3. LED should blink at 2 Hz"
echo "4. You should hear a constant tone"
echo ""
echo "Building..."

make -f Makefile_CodecDiag clean
make -f Makefile_CodecDiag

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful! Binary size:"
    ls -lh build/AudioCodecDiagnostic.bin
    echo ""
    echo "Make sure Daisy is in DFU mode (hold BOOT + press RESET)"
    echo "Press Enter when ready to upload..."
    read

    dfu-util -a 0 -s 0x08000000:leave -D build/AudioCodecDiagnostic.bin

    echo ""
    echo "================================"
    echo "EXPECTED RESULTS:"
    echo "- LED blinking = Code running ✓"
    echo "- 440 Hz tone = Codec working ✓"
    echo "- LED blinks but no sound = Codec init failed ✗"
    echo "================================"
else
    echo ""
    echo "Build failed! Check errors above."
    exit 1
fi
