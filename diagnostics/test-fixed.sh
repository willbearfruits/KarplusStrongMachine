#!/bin/bash
# Build and upload fixed Karplus-Strong version

cd /home/glitches/Documents/KarplusStrongMachine

echo "================================"
echo "KARPLUS-STRONG FIXED VERSION"
echo "================================"
echo ""
echo "This version includes:"
echo "- 100ms delay after hw.Init() for codec stabilization"
echo "- 50ms delay after StartAudio() for DMA stabilization"
echo "- Clean initialization sequence"
echo ""
echo "Building..."

make -f Makefile_Fixed clean
make -f Makefile_Fixed

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful! Binary size:"
    ls -lh build/KarplusStrongMachine_Fixed.bin
    echo ""
    echo "Make sure Daisy is in DFU mode (hold BOOT + press RESET)"
    echo "Press Enter when ready to upload..."
    read

    dfu-util -a 0 -s 0x08000000:leave -D build/KarplusStrongMachine_Fixed.bin

    echo ""
    echo "================================"
    echo "Upload complete!"
    echo "LED should blink on each pluck trigger"
    echo "You should hear Karplus-Strong synthesis"
    echo "================================"
else
    echo ""
    echo "Build failed! Check errors above."
    exit 1
fi
