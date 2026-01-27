# Core Functionality

## AppImage Discovery

- Scan a directory for .AppImage files (case-insensitive detection)
- Support recursive scanning of subdirectories (one level deep for grouped view)
- Automatically detect and filter AppImage files by extension

## AppImage Launching

- Launch AppImage files with a single click
- Automatically make files executable before launching (chmod +x)
- Launch in background (fork/exec) without blocking UI
- Handle FUSE2 requirements

## UI/Display Modes

- Flat View: Show all AppImages in a grid layout
- Grouped View: Organize AppImages by their parent folder
- Toggle switch to switch between views
- Grid layout with tiles (2 columns minimum)
- Each tile shows: icon (96x96px) + filename with word wrap

## Metadata Extraction

- Extract icons from AppImage files (.DirIcon or from .desktop file)
- Parse .desktop files for Name, Comment, Icon fields
- Cache extracted metadata (SHA256 hash-based cache directory)
- Background thread processing to avoid UI blocking
- Thread-safe cache with mutex protection

## Directory Navigation

- Path entry field to manually enter directory
- "Open Folder..." button with file chooser dialog
- "Refresh" button to rescan current directory
- Start in user's home directory by default
- Remember last opened directory (via GSettings)

## File Monitoring

- Auto-detect changes in watched directory (GFileMonitor)
- Support monitoring subdirectories when in grouped mode
- Automatically refresh UI when files are added/removed

## Drag & Drop Support

- Accept .AppImage files dropped into window
- Copy dropped files to current directory
- Show visual feedback during drag operation

## Context Menu (Right-click on AppImage)

"Open" - Launch the AppImage
"Show in Folder" - Open file manager at location
"Properties" - Show file info dialog

## FUSE2 Management

- Detect if FUSE2 is installed
- Show "Install Requirements" button when missing
- Auto-install FUSE2 using system package manager (apt/dnf/yum/pacman/zypper)
- Use pkexec for privilege escalation
- Fallback to extract mode if FUSE unavailable

## Preferences/Settings (GSettings)

- Persist last opened directory
- Persist grouped/flat view preference
- Schema: com.github.gtkappfolder.gschema.xml

## Visual Design

- GTK3: Traditional GTK3 widgets, manual layout
- GTK4: Modern libadwaita styling, AdwApplicationWindow, AdwHeaderBar
- Responsive grid layout that adapts to window size
- Status bar showing hints/messages
- Clean, minimal interface focused on AppImages

# Technical Requirements

## Architecture:

- Modular C++17 codebase with separate core/system/ui layers
- Platform-independent core logic (shared between GTK3/GTK4)
- GTK-specific UI code in separate directories:
  - src/ui/gtk3/ - GTK3 implementation
  - src/ui/gtk4/ - GTK4 implementation
  - core - AppImage scanning, metadata
  - system - Process management, file monitoring, preferences

## Build System:

- CMake 3.15+ with option to choose GTK version
- -DUSE_GTK4=ON (default) or -DUSE_GTK4=OFF
- Conditional compilation using preprocessor directives

## Dependencies:

- C++17 compiler
- GTK3 (3.24+) OR GTK4 (4.0+)
- libadwaita 1.0+ (GTK4 only)
- GLib/GIO 2.0+
- OpenSSL (for SHA256 hashing)
- Optional: FUSE v2 for AppImage mounting

## Performance:

- Non-blocking metadata extraction (background threads)
- Efficient caching to avoid re-extracting metadata
- Lazy loading of icons
- Minimal memory footprint

## Error Handling:

- Graceful fallback when FUSE is missing
- Handle filesystem errors (permissions, missing dirs)
- Validate AppImage files before launching
- User-friendly error messages
