# GTK AppImage Browser - AI Agent Instructions

## Project Overview
A GTK4/libadwaita desktop application for browsing, organizing, and launching AppImage files. Key features include folder grouping, metadata extraction from AppImage `.desktop` files, drag-and-drop support, and automatic directory monitoring.

**Tech Stack:** C++17 · GTK4 · libadwaita · GIO file monitoring · CMake

## Architecture

### Main Components
- **AppImageBrowser class** (`src/AppImageBrowser.cpp/h`): Core application logic, UI creation, file scanning, and launch handling
- **main.cpp**: Minimal entry point that initializes adwaita and runs the application
- **CMake build**: Handles GTK4/libadwaita/GIO dependency resolution via pkg-config

### Critical Data Flows
1. **Directory Monitoring**: `start_dir_monitor()` watches filesystem; changes trigger `on_dir_changed()` → `scan_directory()` → `populate_list()`
2. **AppImage Launch**: `on_appimage_clicked()` → `launch_appimage()` → `GSubprocess` with FUSE fallback handling
3. **UI Rendering**: `populate_list()` generates either grouped view (`groups_box`) or flat view (`flow_box`) based on `group_by_folder` state
4. **Metadata Caching**: Async background thread (`metadata_thread_func`) extracts `.desktop` file data and caches icons in `~/.cache/gtkappfolder/`

### View Modes
- **Grouped (default)**: Folder-based sections with `GtkExpander` containers; each section is a flowbox
- **Flat**: Single flowbox showing all AppImages; legacy but maintained for compatibility

## Build & Development

### Build Commands
```bash
mkdir -p build && cd build
cmake ..
make
./bin/GtkAppFolder
```

### Key Build Details
- **C++ Standard**: C++17 (required for `std::filesystem`)
- **Output**: `build/bin/GtkAppFolder` executable
- **GSettings Schema**: Auto-compiled from `schemas/com.github.gtkappfolder.gschema.xml` during build
- **Compile Commands**: Generated at `build/compile_commands.json` for IDE support

### Test Workflow
Create test AppImages or symlink existing ones to a test directory, then:
1. Launch app and navigate to test directory
2. Monitor console output: `./bin/GtkAppFolder` prints launch status and errors
3. Check `~/.cache/gtkappfolder/` for cached metadata

## Code Patterns & Conventions

### Signal Handlers & Callbacks
All GTK signal callbacks are static member functions with signature `callback(GtkWidget* widget, gpointer user_data)`:
```cpp
static void on_appimage_clicked(GtkButton* button, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    // use browser->...
}
```
Register via `g_signal_connect(widget, "signal-name", G_CALLBACK(on_event), this)`.

### Error Handling in Launch
`launch_appimage()` uses `GSubprocess` for robust spawning:
- Checks FUSE2 availability; sets `APPIMAGE_EXTRACT_AND_RUN=1` fallback if missing
- Prints errors to stderr; sets status label UI message
- Does **not** wait for subprocess; unref immediately after spawn to allow async execution

### File Monitoring
- `start_dir_monitor()`: Creates single monitor for top-level directory
- `start_subdir_monitors()`: Creates monitors for each subfolder (for grouped view)
- All monitors call `on_dir_changed()` on file create/delete/modify events

### Metadata Extraction
Desktop file parsing (`parse_desktop_file()`):
- Searches for `.desktop` in AppImage root via `find_desktop_file()`
- Extracts `Name=` and `Comment=` fields
- Icon path stored; extracted to cache directory asynchronously
- Results cached in memory (`cache_`) and stored on disk for faster relaunch

### Settings Persistence
Uses GSettings (`com.github.gtkappfolder`) via `init_settings()`:
- Persists `group-by-folder` toggle state
- Schema defined in `schemas/com.github.gtkappfolder.gschema.xml`

## Integration Points

### External Dependencies
- **GTK4 / libadwaita**: UI framework; widgets like `GtkSwitch`, `GtkExpander`, `AdwHeaderBar`
- **GIO**: File monitoring (`GFileMonitor`) and subprocess spawning (`GSubprocess`)
- **libc**: File operations (`stat`, `chmod`), environment (`getcwd`, `getenv`)

### Subprocess Launching
When launching AppImages:
- `GSubprocessLauncher` spawns with optional `APPIMAGE_EXTRACT_AND_RUN` env var
- Parent process does **not** wait; AppImage runs detached
- Subprocess inherits parent's environment and working directory

## Common Modifications

### Changing UI Layout
- Header bar items: Add to `adw_header_bar_pack_start/end()` in `create_ui()`
- Main view: Modify vbox children in `create_ui()` or toggle `content_stack` visibility
- Grouped sections: Modify tile building in `build_appimage_tile()` or section layout in `populate_list()`

### Modifying Launch Behavior
- Edit `launch_appimage()` to change subprocess flags, environment variables, or wait behavior
- Update status messages via `set_status()` to reflect UI changes
- Add post-launch hooks by capturing `GSubprocess*` and using `g_subprocess_wait_async()`

### Adding New Settings
1. Add new key-value to `schemas/com.github.gtkappfolder.gschema.xml`
2. Recompile: `cmake .. && make` (schema auto-compiles)
3. Load in `init_settings()` via `g_settings_get_*()` and store in class member
4. Update UI control (e.g., toggle) via `g_signal_connect()`

## File Structure Quick Reference
- `src/main.cpp`: Entry point (10 lines)
- `src/AppImageBrowser.h`: Class definition and members
- `src/AppImageBrowser.cpp`: ~1170 lines covering UI, file operations, callbacks
- `CMakeLists.txt`: Build configuration
- `schemas/com.github.gtkappfolder.gschema.xml`: GSettings schema
- `README.md`: User-facing documentation
