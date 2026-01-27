# Build Instructions

GtkAppFolder supports both GTK3 and GTK4. You can choose which version to build with using CMake options.

## Building with GTK4 (Default)

GTK4 with libadwaita is the default and recommended version:

```bash
mkdir build && cd build
cmake .. -DUSE_GTK4=ON
cmake --build . -j4
./bin/GtkAppFolder
```

## Building with GTK3

To build the GTK3 version:

```bash
mkdir build-gtk3 && cd build-gtk3
cmake .. -DUSE_GTK4=OFF
cmake --build . -j4
./bin/GtkAppFolder
```

## Dependencies

### GTK4 Build Dependencies

- GTK 4.0+
- libadwaita 1.0+
- GLib/GIO 2.0+
- OpenSSL (for metadata caching)
- C++17 compiler

**Install on Debian/Ubuntu:**

```bash
sudo apt-get install libgtk-4-dev libadwaita-1-dev libssl-dev
```

**Install on Fedora:**

```bash
sudo dnf install gtk4-devel libadwaita-devel openssl-devel
```

### GTK3 Build Dependencies

- GTK 3.0+
- GLib/GIO 2.0+
- OpenSSL (for metadata caching)
- C++17 compiler

**Install on Debian/Ubuntu:**

```bash
sudo apt-get install libgtk-3-dev libssl-dev
```

**Install on Fedora:**

```bash
sudo dnf install gtk3-devel openssl-devel
```

## Modular Architecture

The codebase is now modularized into:

- **`src/core/`** - Platform-independent core logic (AppImage scanning, metadata)
- **`src/system/`** - System services (process management, file monitoring, preferences)
- **`src/AppImageBrowser.*`** - UI layer (GTK3/GTK4 specific)

The core and system modules are shared between GTK3 and GTK4 builds, with only the UI layer having toolkit-specific code.

## Quick Reference

| Command                   | Description                  |
| ------------------------- | ---------------------------- |
| `cmake .. -DUSE_GTK4=ON`  | Build with GTK4 (default)    |
| `cmake .. -DUSE_GTK4=OFF` | Build with GTK3              |
| `cmake --build . -j4`     | Compile with 4 parallel jobs |
| `./bin/GtkAppFolder`      | Run the application          |
