# GTK AppImage Browser

A lightweight GTK3 app to browse and launch AppImage files with a modern header bar UI. It can optionally install the common runtime dependency (FUSE) and falls back to extract-and-run when FUSE is unavailable.

## Features

- Browse .AppImage files in current or chosen folder
- Modern GTK HeaderBar, folder picker, and status line
- One-click "Install Requirements" (uses pkexec) for FUSE
- Click to launch AppImages; auto-makes them executable
- Fallback: runs with APPIMAGE_EXTRACT_AND_RUN if FUSE is missing

## Requirements

- GTK3 development libraries (includes GLib/GIO)
- CMake 3.15 or higher
- C++17 compatible compiler

## Building

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get install -y build-essential cmake libgtk-3-dev

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
3. Click "Open Folder…" or edit the path and press Enter
4. Click on any AppImage file to launch it
5. If an AppImage fails due to missing FUSE, click "Install Requirements" (uses `pkexec`) or rely on the built-in extract-and-run fallback (slower)

## Notes

- AppImages often require FUSE v2 (`libfuse2`). On some distros this is not preinstalled. The app can install it via `pkexec` for common package managers (apt, dnf, yum, pacman, zypper).
- When FUSE is unavailable, the app sets `APPIMAGE_EXTRACT_AND_RUN=1` to run without mounting. This is slower but avoids system changes.

## License

MIT License
