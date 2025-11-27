/**
 * Main application logic for Daisy Seed Web Flasher
 */

import { DFUDevice } from './dfu.js';

// State
let dfuDevice = null;
let firmwareData = null;

// Firmware URL
const FIRMWARE_URL = 'firmware/DigitalKalimba.bin';

// DOM elements
const elements = {
    connectBtn: document.getElementById('connect-btn'),
    flashBtn: document.getElementById('flash-btn'),
    deviceStatus: document.getElementById('device-status'),
    firmwareStatus: document.getElementById('firmware-status'),
    progressContainer: document.getElementById('progress-container'),
    progressFill: document.getElementById('progress-fill'),
    progressText: document.getElementById('progress-text'),
    logOutput: document.getElementById('log-output'),
    customFirmware: document.getElementById('custom-firmware'),
    customFilename: document.getElementById('custom-filename'),
    compatibilityWarning: document.getElementById('compatibility-warning')
};

/**
 * Initialize application
 */
function init() {
    // Check WebUSB support
    if (!navigator.usb) {
        elements.compatibilityWarning.classList.remove('hidden');
        elements.connectBtn.disabled = true;
        log('Browser does not support WebUSB', 'error');
        return;
    }

    // Event listeners
    elements.connectBtn.addEventListener('click', handleConnect);
    elements.flashBtn.addEventListener('click', handleFlash);
    elements.customFirmware.addEventListener('change', handleCustomFirmware);

    log('Multi-Scale Synthesizer - Ready to flash!', 'info');
    log('Ready to connect to Daisy Seed', 'info');
}

/**
 * Handle device connection
 */
async function handleConnect() {
    try {
        elements.connectBtn.disabled = true;
        log('Requesting device connection...', 'info');

        dfuDevice = new DFUDevice();
        const deviceInfo = await dfuDevice.connect();

        log(`Connected to ${deviceInfo.productName || 'STM32 Device'}`, 'success');
        log(`Vendor: 0x${deviceInfo.vendorId.toString(16).padStart(4, '0')}`, 'info');
        log(`Product: 0x${deviceInfo.productId.toString(16).padStart(4, '0')}`, 'info');

        elements.deviceStatus.textContent = 'Connected';
        elements.deviceStatus.style.color = 'var(--success-color)';
        elements.connectBtn.textContent = '✓ Connected';
        elements.flashBtn.disabled = false;

    } catch (error) {
        log(`Connection failed: ${error.message}`, 'error');
        elements.deviceStatus.textContent = 'Connection Failed';
        elements.deviceStatus.style.color = 'var(--danger-color)';
        elements.connectBtn.disabled = false;
        dfuDevice = null;
    }
}

/**
 * Handle firmware flashing
 */
async function handleFlash() {
    if (!dfuDevice) {
        log('No device connected', 'error');
        return;
    }

    try {
        elements.flashBtn.disabled = true;
        elements.connectBtn.disabled = true;

        // Load firmware if not already loaded
        if (!firmwareData) {
            log('Loading firmware...', 'info');
            await loadFirmware();
        }

        log('Starting flash process...', 'warning');
        elements.progressContainer.classList.remove('hidden');

        // Flash firmware
        await dfuDevice.flash(firmwareData, updateProgress);

        log('Flash completed successfully! 🎉', 'success');
        log('Press RESET button on your Daisy Seed to start the firmware', 'success');

    } catch (error) {
        log(`Flash failed: ${error.message}`, 'error');
        log('Try putting the device back in bootloader mode and reconnecting', 'warning');
    } finally {
        elements.flashBtn.disabled = false;
        elements.connectBtn.disabled = false;
    }
}

/**
 * Load firmware binary
 */
async function loadFirmware() {
    try {
        const response = await fetch(FIRMWARE_URL);

        if (!response.ok) {
            throw new Error(`HTTP error ${response.status}`);
        }

        const blob = await response.blob();
        firmwareData = await blob.arrayBuffer();

        const sizeKB = (firmwareData.byteLength / 1024).toFixed(2);
        log(`Firmware loaded: ${sizeKB} KB`, 'success');
        updateFirmwareStatus();

    } catch (error) {
        // Firmware files not available - probably in development
        log('Pre-compiled firmware not available', 'warning');
        log('Please compile the firmware or upload a custom .bin file', 'info');
        throw new Error('Firmware not found. Please compile or upload a .bin file');
    }
}

/**
 * Handle custom firmware upload
 */
async function handleCustomFirmware(event) {
    const file = event.target.files[0];
    if (!file) return;

    if (!file.name.endsWith('.bin')) {
        log('Please select a .bin file', 'error');
        return;
    }

    try {
        firmwareData = await file.arrayBuffer();
        elements.customFilename.textContent = file.name;

        const sizeKB = (firmwareData.byteLength / 1024).toFixed(2);
        log(`Custom firmware loaded: ${file.name} (${sizeKB} KB)`, 'success');
        updateFirmwareStatus();

    } catch (error) {
        log(`Failed to load custom firmware: ${error.message}`, 'error');
    }
}

/**
 * Update progress during flashing
 */
function updateProgress(progress) {
    elements.progressFill.style.width = `${progress.percent}%`;
    elements.progressText.textContent = `${progress.percent}% - ${progress.message}`;

    log(`${progress.stage}: ${progress.message}`, 'info');
}

/**
 * Update firmware status display
 */
function updateFirmwareStatus() {
    if (firmwareData) {
        const sizeKB = (firmwareData.byteLength / 1024).toFixed(2);
        elements.firmwareStatus.textContent = `Loaded (${sizeKB} KB)`;
        elements.firmwareStatus.style.color = 'var(--success-color)';
    } else {
        elements.firmwareStatus.textContent = 'Not Loaded';
        elements.firmwareStatus.style.color = 'var(--text-muted)';
    }
}

/**
 * Log message to output
 */
function log(message, type = 'info') {
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    const timestamp = new Date().toLocaleTimeString();
    entry.textContent = `[${timestamp}] ${message}`;
    elements.logOutput.appendChild(entry);
    elements.logOutput.scrollTop = elements.logOutput.scrollHeight;
}

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
