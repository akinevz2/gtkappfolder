#!/bin/bash

echo "Installing dependencies for RHEL/CentOS/Fedora..."

# Determine if we should use yum or dnf
if command -v dnf &> /dev/null; then
    PKG_MGR="dnf"
else
    PKG_MGR="yum"
fi

# Install dependencies
sudo $PKG_MGR install -y \
    gcc-c++ \
    make \
    cmake \
    ninja-build \
    pkgconfig \
    gtk3-devel \
    gtk4-devel \
    libadwaita-devel \
    openssl-devel

if [ $? -eq 0 ]; then
    echo "✓ All dependencies installed successfully!"
else
    echo "✗ Error: Failed to install dependencies"
    exit 1
fi
