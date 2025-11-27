#!/bin/bash

echo "========================================="
echo "  KARPLUS-STRONG MACHINE - UPLOAD"
echo "========================================="
echo ""

# Check if binary exists
if [ ! -f "build/KarplusStrongMachine.bin" ]; then
    echo "❌ ERROR: Binary not found!"
    echo "Please build first: ./build.sh"
    exit 1
fi

echo "✓ Found binary"
echo ""

# Check if Daisy is in bootloader mode
echo "Checking for Daisy Seed in bootloader mode..."
lsusb | grep -q "0483:df11"

if [ $? -ne 0 ]; then
    echo ""
    echo "⚠️  WARNING: Daisy Seed not detected!"
    echo ""
    echo "Put Daisy in bootloader mode:"
    echo "  1. Hold BOOT button"
    echo "  2. Press RESET button"
    echo "  3. Release BOOT button"
    echo "  4. LED should pulse slowly"
    echo ""
    read -p "Press Enter when ready, or Ctrl+C to cancel..."
fi

# Upload
echo ""
echo "Uploading..."
make program-dfu

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "✅ UPLOAD SUCCESSFUL!"
    echo "========================================="
    echo ""
    echo "Press RESET on Daisy to start"
    echo ""
else
    echo ""
    echo "❌ UPLOAD FAILED"
    exit 1
fi
