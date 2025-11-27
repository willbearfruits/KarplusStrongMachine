#!/bin/bash
# Upload script for Digital Kalimba

set -e

echo "🎵 ======================================="
echo "🎵  Uploading Digital Kalimba to Daisy"
echo "🎵 ======================================="
echo ""

# Check if binary exists
if [ ! -f "build/DigitalKalimba.bin" ]; then
    echo "❌ ERROR: Binary not found. Run ./build-kalimba.sh first"
    exit 1
fi

echo "📦 Binary found: build/DigitalKalimba.bin"
echo ""
echo "⚠️  BOOTLOADER MODE:"
echo "   1. Hold BOOT button on Daisy Seed"
echo "   2. Press RESET button (while holding BOOT)"
echo "   3. Release BOOT button"
echo "   4. LED should pulse slowly"
echo ""
read -p "Press ENTER when Daisy is in bootloader mode..."

echo ""
echo "📤 Uploading via DFU..."
make -f Makefile.kalimba program-dfu

echo ""
echo "✅ UPLOAD COMPLETE!"
echo ""
echo "🎹 Press RESET button on Daisy to start the kalimba"
echo ""
echo "🎵 BUTTON LAYOUT (G Major Pentatonic):"
echo "   Button 1 (D15): G4 (392 Hz) - Center"
echo "   Button 2 (D16): A3 (220 Hz) - Left 1"
echo "   Button 3 (D17): B4 (494 Hz) - Right 1"
echo "   Button 4 (D18): D4 (294 Hz) - Left 2"
echo "   Button 5 (D19): E4 (330 Hz) - Right 2"
echo "   Button 6 (D20): G3 (196 Hz) - Left 3 (lowest)"
echo "   Button 7 (D21): A4 (440 Hz) - Right 3"
echo ""
echo "🎛️  CONTROLS:"
echo "   A0: Global Brightness"
echo "   A1: Global Decay/Sustain"
echo "   A2: Reverb Amount"
echo "   A3: Reverb Size"
echo "   A4: LFO Rate"
echo "   A5: LFO Depth"
