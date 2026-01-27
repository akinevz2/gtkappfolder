#pragma once

#include <string>

namespace System {

/**
 * Manages process execution for AppImages and system utilities.
 * Platform-independent process management.
 */
class ProcessManager {
public:
    /**
     * Launches an AppImage file.
     * Makes the file executable if needed and runs it in the background.
     * 
     * @param path Full path to the AppImage file
     * @return true if launch was successful, false otherwise
     */
    static bool launch_appimage(const std::string& path);
    
    /**
     * Checks if FUSE2 is installed on the system.
     * 
     * @return true if FUSE2 is available, false otherwise
     */
    static bool has_fuse2();
    
    /**
     * Installs FUSE2 requirements using the appropriate package manager.
     * Detects the system's package manager and runs the installation.
     * 
     * @return true if installation was initiated successfully, false otherwise
     */
    static bool install_fuse2_requirements();
    
    /**
     * Opens a file manager at the specified path.
     * 
     * @param path Directory or file path to open
     * @return true if successful, false otherwise
     */
    static bool open_in_file_manager(const std::string& path);
    
private:
    /**
     * Makes a file executable.
     * 
     * @param path Path to the file
     * @return true if successful, false otherwise
     */
    static bool make_executable(const std::string& path);
    
    /**
     * Detects the available package manager on the system.
     * 
     * @return Package manager command (apt-get, dnf, yum, pacman, zypper) or empty string
     */
    static std::string detect_package_manager();
};

} // namespace System
