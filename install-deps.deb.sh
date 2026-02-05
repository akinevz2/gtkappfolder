#!/bin/bash

echo "Installing dependencies for Debian/Ubuntu..."

# Update package list
sudo apt-get update

# Install dependencies
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libgtk-3-dev \
    libgtk-4-dev \
    libadwaita-1-dev \
    libssl-dev

if [ $? -eq 0 ]; then
    echo "✓ All dependencies installed successfully!"
else
    echo "✗ Error: Failed to install dependencies"
    exit 1
fi
