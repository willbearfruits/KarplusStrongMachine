#!/bin/bash
# Combined build and upload for Digital Kalimba

set -e

echo "🎵 ================================================"
echo "🎵  Digital Kalimba - Build & Upload to Daisy"
echo "🎵 ================================================"
echo ""

# Build
./build-kalimba.sh

echo ""
echo "==============================================="
echo ""

# Upload
./upload-kalimba.sh
