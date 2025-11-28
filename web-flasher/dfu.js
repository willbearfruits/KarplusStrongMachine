/**
 * STM32 DFU (Device Firmware Update) Implementation for WebUSB
 * Based on the DFU 1.1 specification
 * Adapted for STM32H750 (Daisy Seed)
 */

// DFU Commands
const DFU_COMMANDS = {
    DETACH: 0,
    DNLOAD: 1,
    UPLOAD: 2,
    GETSTATUS: 3,
    CLRSTATUS: 4,
    GETSTATE: 5,
    ABORT: 6
};

// DFU States
const DFU_STATE = {
    APP_IDLE: 0,
    APP_DETACH: 1,
    DFU_IDLE: 2,
    DFU_DNLOAD_SYNC: 3,
    DFU_DNBUSY: 4,
    DFU_DNLOAD_IDLE: 5,
    DFU_MANIFEST_SYNC: 6,
    DFU_MANIFEST: 7,
    DFU_MANIFEST_WAIT_RESET: 8,
    DFU_UPLOAD_IDLE: 9,
    DFU_ERROR: 10
};

// DFU Status
const DFU_STATUS = {
    OK: 0x00,
    ERR_TARGET: 0x01,
    ERR_FILE: 0x02,
    ERR_WRITE: 0x03,
    ERR_ERASE: 0x04,
    ERR_CHECK_ERASED: 0x05,
    ERR_PROG: 0x06,
    ERR_VERIFY: 0x07,
    ERR_ADDRESS: 0x08,
    ERR_NOTDONE: 0x09,
    ERR_FIRMWARE: 0x0A,
    ERR_VENDOR: 0x0B,
    ERR_USBR: 0x0C,
    ERR_POR: 0x0D,
    ERR_UNKNOWN: 0x0E,
    ERR_STALLEDPKT: 0x0F
};

class DFUDevice {
    constructor() {
        this.device = null;
        this.interfaceNumber = 0;
        this.transferSize = 1024; // Reduced to 1024 for better stability
        this.startAddress = 0x08000000; // STM32H750 Flash start address
    }

    /**
     * Request and connect to a DFU device
     */
    async connect() {
        try {
            // Request USB device with STM32 DFU vendor/product IDs
            this.device = await navigator.usb.requestDevice({
                filters: [
                    { vendorId: 0x0483, productId: 0xdf11 } // STM32 in DFU mode
                ]
            });

            await this.device.open();

            // Find DFU interface
            const configInterfaces = this.device.configuration.interfaces;
            let dfuInterface = null;

            for (const iface of configInterfaces) {
                const alt = iface.alternates[0];
                if (alt.interfaceClass === 0xFE && alt.interfaceSubclass === 0x01) {
                    dfuInterface = iface;
                    this.interfaceNumber = iface.interfaceNumber;
                    break;
                }
            }

            if (!dfuInterface) {
                throw new Error('DFU interface not found');
            }

            await this.device.claimInterface(this.interfaceNumber);

            // Clear any previous DFU errors
            await this.clearStatus();

            return {
                vendorId: this.device.vendorId,
                productId: this.device.productId,
                manufacturerName: this.device.manufacturerName,
                productName: this.device.productName
            };

        } catch (error) {
            throw new Error(`Failed to connect: ${error.message}`);
        }
    }

    /**
     * Disconnect from device
     */
    async disconnect() {
        if (this.device) {
            try {
                await this.device.releaseInterface(this.interfaceNumber);
                await this.device.close();
            } catch (error) {
                console.error('Error disconnecting:', error);
            }
            this.device = null;
        }
    }

    /**
     * Get DFU status
     */
    async getStatus() {
        const response = await this.device.controlTransferIn({
            requestType: 'class',
            recipient: 'interface',
            request: DFU_COMMANDS.GETSTATUS,
            value: 0,
            index: this.interfaceNumber
        }, 6);

        if (response.status !== 'ok') {
            throw new Error('Failed to get status');
        }

        const data = new Uint8Array(response.data.buffer);
        return {
            status: data[0],
            pollTimeout: data[1] | (data[2] << 8) | (data[3] << 16),
            state: data[4],
            string: data[5]
        };
    }

    /**
     * Clear DFU status/error
     */
    async clearStatus() {
        await this.device.controlTransferOut({
            requestType: 'class',
            recipient: 'interface',
            request: DFU_COMMANDS.CLRSTATUS,
            value: 0,
            index: this.interfaceNumber
        });
    }

    /**
     * Wait for device to be ready
     */
    async waitForReady() {
        let status = await this.getStatus();
        let attempts = 0;
        const maxAttempts = 500; // Increased from 100 to allow for slower operations

        // Accept either IDLE or DNLOAD_IDLE as "Ready" states
        // IDLE: Ready for new command sequence
        // DNLOAD_IDLE: Ready for next block in current sequence
        while (status.state !== DFU_STATE.DFU_IDLE && 
               status.state !== DFU_STATE.DFU_DNLOAD_IDLE && 
               attempts < maxAttempts) {
            
            if (status.state === DFU_STATE.DFU_ERROR) {
                await this.clearStatus();
            }

            // Wait for poll timeout or default minimum to prevent tight loop
            const waitTime = status.pollTimeout > 0 ? status.pollTimeout : 10;
            
            // Debug logging for long operations (like erase)
            if (attempts % 10 === 0) {
                console.log(`Waiting for ready... State: ${status.state}, Status: ${status.status}, Wait: ${waitTime}ms, Attempt: ${attempts}`);
            }

            await new Promise(resolve => setTimeout(resolve, waitTime));

            status = await this.getStatus();
            attempts++;
        }

        if (attempts >= maxAttempts) {
            throw new Error(`Timeout waiting for device to be ready (State: ${status.state})`);
        }

        return status;
    }

    /**
     * Download (write) data to device
     */
    async download(data, blockNum = 0) {
        await this.device.controlTransferOut({
            requestType: 'class',
            recipient: 'interface',
            request: DFU_COMMANDS.DNLOAD,
            value: blockNum,
            index: this.interfaceNumber
        }, data);
    }

    /**
     * Set address pointer (STM32-specific)
     */
    async setAddress(address) {
        const command = new Uint8Array(5);
        command[0] = 0x21; // Set Address Pointer command
        command[1] = address & 0xFF;
        command[2] = (address >> 8) & 0xFF;
        command[3] = (address >> 16) & 0xFF;
        command[4] = (address >> 24) & 0xFF;

        await this.download(command, 0);
        await this.waitForReady();
    }

    /**
     * Erase flash (STM32-specific)
     */
    async erase(address) {
        const command = new Uint8Array(5);
        command[0] = 0x41; // Erase command
        command[1] = address & 0xFF;
        command[2] = (address >> 8) & 0xFF;
        command[3] = (address >> 16) & 0xFF;
        command[4] = (address >> 24) & 0xFF;

        await this.download(command, 0);

        // Erase takes longer, wait for completion
        let status = await this.getStatus();
        while (status.state === DFU_STATE.DFU_DNBUSY) {
            await new Promise(resolve => setTimeout(resolve, status.pollTimeout || 100));
            status = await this.getStatus();
        }

        await this.waitForReady();
    }

    /**
     * Flash firmware to device
     */
    async flash(firmwareData, progressCallback) {
        try {
            progressCallback({ stage: 'init', percent: 0, message: 'Initializing...' });

            // Set address to flash start
            await this.setAddress(this.startAddress);
            
            progressCallback({ stage: 'write', percent: 5, message: 'Writing firmware...' });

            // Download firmware in chunks (implicit erase by bootloader)
            const totalBlocks = Math.ceil(firmwareData.byteLength / this.transferSize);
            let blockNum = 2; // Start at block 2 (0 and 1 used for commands)

            for (let offset = 0; offset < firmwareData.byteLength; offset += this.transferSize) {
                const chunk = firmwareData.slice(offset, offset + this.transferSize);
                const chunkArray = new Uint8Array(chunk);

                await this.download(chunkArray, blockNum);
                await this.waitForReady();

                const percent = 10 + Math.floor((offset / firmwareData.byteLength) * 80);
                progressCallback({
                    stage: 'download',
                    percent,
                    message: `Writing block ${blockNum - 1}/${totalBlocks}...`
                });

                blockNum++;
            }

            // Send zero-length download to exit DFU mode
            progressCallback({ stage: 'finalize', percent: 100, message: 'Finalizing...' });
            try {
                await this.download(new Uint8Array(0), 0);
            } catch (e) {
                // Ignore errors here - device might reboot immediately
            }

            // Wait for manifest - but be tolerant of disconnection
            try {
                let status = await this.getStatus();
                while (status.state === DFU_STATE.DFU_MANIFEST_SYNC ||
                       status.state === DFU_STATE.DFU_MANIFEST) {
                    await new Promise(resolve => setTimeout(resolve, 100));
                    status = await this.getStatus();
                }
            } catch (e) {
                // Device disconnected/rebooted - this is expected success!
                console.log('Device rebooted successfully');
            }

            progressCallback({ stage: 'complete', percent: 100, message: 'Flash complete!' });

        } catch (error) {
            throw new Error(`Flash failed: ${error.message}`);
        }
    }
}

export { DFUDevice, DFU_STATE, DFU_STATUS };
