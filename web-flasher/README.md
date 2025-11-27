# Daisy Seed Web Flasher

A browser-based firmware flasher for the Daisy Seed using WebUSB and DFU protocol. No installation required!

## Features

- ✅ **No Installation Required** - Flash directly from your browser
- ✅ **Cross-Platform** - Works on Windows, macOS, and Linux
- ✅ **Easy to Use** - Simple point-and-click interface
- ✅ **Progress Tracking** - Real-time flash progress and status
- ✅ **Pre-built Firmware** - One-click flash of Karplus-Strong Machine or Digital Kalimba
- ✅ **Custom Firmware** - Upload your own .bin files

## Browser Requirements

This tool requires a browser with **WebUSB support**:

- ✅ Chrome/Chromium 61+
- ✅ Edge 79+
- ✅ Opera 48+
- ❌ Firefox (WebUSB not supported)
- ❌ Safari (WebUSB not supported)

## How to Use

### Online (GitHub Pages)

Visit: **https://yourusername.github.io/KarplusStrongMachine/web-flasher/**

### Local Development

1. Start a local web server (required for WebUSB security):
   ```bash
   # Using Python 3
   python3 -m http.server 8000

   # Using Node.js
   npx http-server -p 8000

   # Using PHP
   php -S localhost:8000
   ```

2. Open browser to `http://localhost:8000`

### Flashing Firmware

1. **Put Daisy Seed in DFU Bootloader Mode:**
   - Hold **BOOT** button
   - Press and release **RESET** button
   - Release **BOOT** button
   - LED pulses slowly (bootloader mode active)

2. **Select Firmware:**
   - Choose Karplus-Strong Machine or Digital Kalimba
   - Or upload a custom .bin file

3. **Connect:**
   - Click "Connect to Daisy Seed"
   - Select device from browser popup

4. **Flash:**
   - Click "Flash Firmware"
   - Wait for completion (30-60 seconds)

5. **Done:**
   - Press RESET button on Daisy
   - Your new firmware is running!

## File Structure

```
web-flasher/
├── index.html          # Main HTML interface
├── style.css           # Styling
├── flasher.js          # Application logic
├── dfu.js              # DFU protocol implementation
├── firmware/           # Pre-compiled firmware binaries
│   ├── KarplusStrongMachine.bin
│   └── DigitalKalimba.bin
└── README.md           # This file
```

## Hosting on GitHub Pages

1. **Enable GitHub Pages** in repository settings:
   - Go to Settings → Pages
   - Source: Deploy from a branch
   - Branch: `main` / `web-flasher` directory
   - Save

2. **Add firmware binaries** to `firmware/` directory:
   ```bash
   mkdir -p web-flasher/firmware
   cp build/KarplusStrongMachine.bin web-flasher/firmware/
   cp build/DigitalKalimba.bin web-flasher/firmware/
   ```

3. **Commit and push:**
   ```bash
   git add web-flasher/
   git commit -m "Add web flasher with firmware"
   git push
   ```

4. **Access** at: `https://yourusername.github.io/yourrepo/web-flasher/`

## Security Notes

- WebUSB requires HTTPS or localhost
- User must manually grant USB device access
- Browser has full control over device permissions
- Only works when Daisy is in DFU bootloader mode

## Troubleshooting

**"Browser not supported"**
- Use Chrome, Edge, or Opera (not Firefox/Safari)

**"No device found"**
- Ensure Daisy is in bootloader mode (LED pulsing)
- Try a different USB cable
- Try a different USB port
- Check USB device permissions

**"Flash failed"**
- Put device back in bootloader mode
- Reconnect to device
- Try again

**"Firmware not found"**
- Pre-compiled binaries need to be added to `firmware/` directory
- Or upload a custom .bin file using the Advanced section

## Technical Details

### DFU Protocol

Uses STM32 DFU (Device Firmware Update) protocol v1.1:
- Vendor ID: `0x0483` (STMicroelectronics)
- Product ID: `0xdf11` (STM32 in DFU mode)
- Flash Address: `0x08000000` (STM32H750)
- Transfer Size: 2048 bytes
- Interface Class: `0xFE` (Application Specific)
- Interface Subclass: `0x01` (DFU)

### WebUSB API

- `navigator.usb.requestDevice()` - Request device access
- `controlTransferOut()` - Send DFU commands
- `controlTransferIn()` - Read DFU status

## Credits

Based on:
- [WebDFU](https://github.com/devanlai/webdfu) by devanlai
- [dfu-util](http://dfu-util.sourceforge.net/) specification
- STM32 DFU implementation

## License

MIT License - See parent directory LICENSE file
