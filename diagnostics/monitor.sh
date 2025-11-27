#!/bin/bash
# Monitor Daisy Seed USB Serial Output

echo "Waiting for Daisy to appear on /dev/ttyACM0..."
sleep 2

if [ ! -e /dev/ttyACM0 ]; then
    echo "Error: /dev/ttyACM0 not found"
    echo "Make sure Daisy is connected and running (not in DFU mode)"
    exit 1
fi

echo "Daisy found! Connecting to serial monitor..."
echo "Press Ctrl+C to exit"
echo "=========================================="
echo ""

# Use cat to read serial output (simpler than screen)
cat /dev/ttyACM0
