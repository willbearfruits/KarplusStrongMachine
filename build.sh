#!/bin/bash

echo "========================================="
echo "  KARPLUS-STRONG MACHINE - BUILD"
echo "========================================="
echo ""

# Check if libDaisy exists
if [ ! -d "$HOME/DaisyExamples/libDaisy" ]; then
    echo "❌ ERROR: libDaisy not found"
    exit 1
fi

# Check if DaisySP exists
if [ ! -d "$HOME/DaisyExamples/DaisySP" ]; then
    echo "❌ ERROR: DaisySP not found"
    exit 1
fi

echo "✓ Found libDaisy"
echo "✓ Found DaisySP"
echo ""

# Clean previous build
echo "Cleaning previous build..."
make clean
echo ""

# Build
echo "Building KarplusStrongMachine.cpp..."
echo ""
make -j4

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "✅ BUILD SUCCESSFUL!"
    echo "========================================="
    echo ""
    ls -lh build/KarplusStrongMachine.bin
    echo ""
    echo "To upload:"
    echo "  1. Put Daisy in bootloader mode"
    echo "  2. Run: ./upload.sh"
    echo ""
else
    echo ""
    echo "========================================="
    echo "❌ BUILD FAILED"
    echo "========================================="
    exit 1
fi
