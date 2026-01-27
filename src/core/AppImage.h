#pragma once

#include <string>
#include <vector>
#include <map>

namespace Core {

/**
 * Represents an AppImage file discovered in the file system.
 */
struct AppImage {
    std::string path;      // Full path to the AppImage file
    std::string filename;  // Just the filename (e.g., "MyApp.AppImage")
    
    AppImage(const std::string& p, const std::string& fn)
        : path(p), filename(fn) {}
};

/**
 * AppImage discovery functions.
 * These are platform-independent and shared between GTK3 and GTK4 branches.
 */
class AppImageScanner {
public:
    /**
     * Scans a directory for AppImage files.
     * 
     * @param path Directory path to scan
     * @param recursive If true, scans subdirectories recursively
     * @return Vector of discovered AppImage file paths (full paths)
     */
    static std::vector<std::string> scan_directory(const std::string& path, bool recursive = false);
    
    /**
     * Groups AppImages by their parent directory.
     * 
     * @param appimages List of AppImage paths
     * @return Map of directory path -> list of AppImage paths in that directory
     */
    static std::map<std::string, std::vector<std::string>> group_by_directory(
        const std::vector<std::string>& appimages);
    
    /**
     * Checks if a file is an AppImage based on filename extension.
     * 
     * @param filename The filename to check (can be full path or just filename)
     * @return true if the file ends with .AppImage (case-insensitive)
     */
    static bool is_appimage(const std::string& filename);
    
private:
    static void scan_recursive(const std::string& path, std::vector<std::string>& results);
};

} // namespace Core
