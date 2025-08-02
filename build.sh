#!/bin/bash

# Build script for the Backup and Recovery System

echo "Building Backup and Recovery System..."

# Create build directory
mkdir -p build
cd build || exit 1

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
make -j"$(nproc)"

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable: ./backup_system"
    echo ""
    echo "To install system-wide, run:"
    echo "  sudo make install"
else
    echo "Build failed!"
    exit 1
fi
