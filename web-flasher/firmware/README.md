# Firmware Binaries

Place compiled .bin files here:
- KarplusStrongMachine.bin
- DigitalKalimba.bin

These will be downloaded by the web flasher.

## Building Firmware

From the project root:

```bash
# Build Karplus-Strong Machine
./build.sh
cp build/KarplusStrongMachine.bin web-flasher/firmware/

# Build Digital Kalimba
./build-kalimba.sh
cp build/DigitalKalimba.bin web-flasher/firmware/
```

