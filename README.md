# GTK AppImage Browser

A simple GTK3 application to browse and launch AppImage files in a directory.

## Features

- Browse .AppImage files in current or specified directory
- Simple GUI with file list
- Click to launch AppImage applications
- Refresh functionality to rescan directory
- Automatically makes AppImage files executable before launching

## Requirements

- GTK3 development libraries
- CMake 3.15 or higher
- C++17 compatible compiler

## Building

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get install build-essential cmake libgtk-3-dev

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run the application
./bin/GtkAppFolder
```

## Usage

1. Launch the application
2. The current directory is scanned by default
3. Change the directory path in the text field and press Enter or click Refresh
4. Click on any AppImage file to launch it

## License

MIT License
