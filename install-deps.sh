#!/bin/bash

# Detect package manager and call appropriate script
echo "Detecting package manager..."

if command -v apt-get &> /dev/null; then
    echo "Detected: Debian/Ubuntu (apt)"
    ./install-deps.deb.sh
elif command -v yum &> /dev/null; then
    echo "Detected: RHEL/CentOS/Fedora (yum)"
    ./install-deps.yum.sh
elif command -v dnf &> /dev/null; then
    echo "Detected: Fedora/RHEL 8+ (dnf)"
    ./install-deps.yum.sh
else
    echo "Error: No supported package manager found (apt-get, yum, or dnf)"
    echo "Please install the following packages manually:"
    echo "  - build-essential / gcc-c++ and make"
    echo "  - cmake"
    echo "  - pkg-config"
    echo "  - GTK3 development libraries (gtk3-devel or libgtk-3-dev)"
    exit 1
fi
