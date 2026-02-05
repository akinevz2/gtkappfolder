# GTK AppImage Browser

A modern GTK4/libadwaita application to browse, organize, and launch AppImage files with folder grouping, metadata extraction, and drag-and-drop support.

> **Note:** The main branch uses **GTK4 with libadwaita**. For the legacy GTK3 version, see the [`gtk3`](https://github.com/akinevz2/gtkappfolder/tree/gtk3) branch.

## Features

- **Grouped & Flat Views**: Toggle between folder-grouped display and flat list
- **Smart Metadata**: Extracts icons and names from AppImage `.desktop` files
- **Drag & Drop**: Add AppImages by dropping them into the window
- **Auto File Monitoring**: Automatically updates when folder contents change
- **Modern UI**: GTK4 with libadwaita styling and adaptive layouts
- **FUSE Fallback**: Automatic extract-and-run mode when FUSE is unavailable
- **One-Click Install**: Install FUSE dependencies via pkexec

## Requirements

- GTK4 and libadwaita development libraries
- CMake 3.15 or higher
- C++17 compatible compiler
- Optional: FUSE v2 (`libfuse2`) for faster AppImage mounting

## Building

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get install -y build-essential cmake ninja-build libgtk-4-dev libadwaita-1-dev

# Or use the provided install script
./install-deps.deb.sh

# Create build directory and build with Ninja
mkdir -p build && cd build
cmake -G Ninja ..
ninja

# Run the application
./bin/GtkAppFolder
```

For other distributions, use the appropriate install script (installs Ninja too):
- Fedora/RHEL: `./install-deps.yum.sh`
- General: `./install-deps.sh`

## Usage

1. Launch the application - it starts in your home directory
2. Use "Open Folder…" button or edit the path entry to browse other directories
3. Toggle "Grouped" switch to organize AppImages by folder or view them flat
4. Click any AppImage tile to launch it
5. Right-click for context menu (Open, Show in Folder, Properties)
6. Drag and drop `.AppImage` files to add them to the current folder
7. If FUSE is missing, click "Install Requirements" or rely on automatic extract-and-run

## Notes

- **Grouped View**: Scans the selected directory and one level of subdirectories, organizing AppImages by folder
- **Metadata Cache**: Icons and names are extracted once and cached for performance
- **GSettings**: Preferences (last folder, grouped mode) persist between sessions via GSettings schema
- **FUSE v2**: Required for fast AppImage mounting; the app auto-detects and offers installation
- **Background Threads**: Icon/metadata extraction runs in background threads to keep UI responsive

## License

MIT License
