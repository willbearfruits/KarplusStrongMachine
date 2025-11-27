#!/bin/bash

echo "========================================="
echo "  KARPLUS-STRONG - BUILD & UPLOAD"
echo "========================================="
echo ""

# Build first
./build.sh

if [ $? -eq 0 ]; then
    echo ""
    read -p "Put Daisy in bootloader mode and press Enter..."
    echo ""
    ./upload.sh
else
    echo "Build failed. Cannot upload."
    exit 1
fi
