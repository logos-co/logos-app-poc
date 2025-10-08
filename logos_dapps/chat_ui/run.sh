#!/bin/bash

# Script to build and run the Chat Qt App

# Exit on error
set -e

SCRIPT_DIR=$(dirname "$0")
BUILD_DIR="$SCRIPT_DIR/build"

# Check if build directory exists, create if not
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Run CMake if CMakeCache.txt doesn't exist
if [ ! -f "CMakeCache.txt" ]; then
    echo "Running CMake..."
    cmake ..
fi

# Build the project
echo "Building chat_qt_app..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# Check if build was successful
if [ ! -f "./chat_qt_app" ]; then
    echo "Build failed, executable not found!"
    exit 1
fi

# Run the application
echo "Starting chat_qt_app..."
./chat_qt_app 