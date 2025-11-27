#!/bin/bash
# Build script for Digital Kalimba

set -e  # Exit on error

echo "🎵 ======================================="
echo "🎵  Building Digital Kalimba for Daisy"
echo "🎵 ======================================="
echo ""

# Check if libDaisy and DaisySP exist
if [ ! -d "$HOME/DaisyExamples/libDaisy" ]; then
    echo "❌ ERROR: libDaisy not found at $HOME/DaisyExamples/libDaisy"
    echo "   Please install libDaisy first"
    exit 1
fi

if [ ! -d "$HOME/DaisyExamples/DaisySP" ]; then
    echo "❌ ERROR: DaisySP not found at $HOME/DaisyExamples/DaisySP"
    echo "   Please install DaisySP first"
    exit 1
fi

echo "✓ Dependencies found"
echo ""

# Clean and build
echo "🧹 Cleaning previous build..."
make -f Makefile.kalimba clean

echo ""
echo "🔨 Compiling Digital Kalimba..."
make -f Makefile.kalimba -j4

echo ""
if [ -f "build/DigitalKalimba.bin" ]; then
    echo "✅ BUILD SUCCESSFUL!"
    echo ""
    ls -lh build/DigitalKalimba.bin
    echo ""
    echo "📊 Binary size: $(du -h build/DigitalKalimba.bin | cut -f1)"
    echo ""
    echo "Ready to upload! Run: ./upload-kalimba.sh"
else
    echo "❌ Build failed - binary not found"
    exit 1
fi
