#!/bin/bash

rm bootloader.bin firmware.bin partitions.bin
# Navigate to the parent directory and then to .pio/build/esp32-*
cd ../.pio/build/esp32-* || { echo "Directory not found"; exit 1; }

# Copy the required files to the current directory
cp bootloader.bin firmware.bin partitions.bin ../../../release || { echo "File(s) not found"; exit 1; }

# Navigate back to the original directory
cd ../../../release

# Create a zip file with the copied files
zip SensorNode_Firmware.zip bootloader.bin firmware.bin partitions.bin

# Remove the copied files from the current directory to keep it clean (optional)
rm bootloader.bin firmware.bin partitions.bin

echo "Firmware files have been zipped into SensorNode_Firmware.zip"