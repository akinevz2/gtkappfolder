#pragma once

#include <string>
#include <unordered_map>
#include <ctime>
#include <mutex>

namespace Core {

/**
 * Cached metadata information for an AppImage.
 * Includes name, description, icon path, and modification time.
 */
struct CachedInfo {
    std::string name;       // Application name (from .desktop Name=)
    std::string comment;    // Description (from .desktop Comment=)
    std::string icon_path;  // Path to extracted icon file
    std::time_t mtime{0};   // File modification time for cache validation
};

/**
 * AppImage metadata extraction and caching.
 * Handles icon extraction, .desktop file parsing, and cache management.
 */
class AppImageMetadata {
public:
    AppImageMetadata() = default;
    
    /**
     * Retrieves cached metadata for an AppImage.
     * 
     * @param path Full path to the AppImage file
     * @return Pointer to cached info if available, nullptr otherwise
     */
    const CachedInfo* get_cached_info(const std::string& path);
    
    /**
     * Stores metadata in the cache.
     * Thread-safe operation.
     * 
     * @param path Full path to the AppImage file
     * @param info Metadata to cache
     */
    void cache_info(const std::string& path, const CachedInfo& info);
    
    /**
     * Extracts icon from an AppImage file.
     * This mounts the AppImage, finds the icon file, and copies it to cache.
     * 
     * @param path Full path to the AppImage file
     * @return Path to extracted icon, or empty string on failure
     */
    static std::string extract_appimage_icon(const std::string& path);
    
    /**
     * Finds a .desktop file within an extracted AppImage directory.
     * 
     * @param root Root directory to search (e.g., squashfs-root)
     * @param out_path Output parameter for the found .desktop file path
     * @return true if found, false otherwise
     */
    static bool find_desktop_file(const std::string& root, std::string& out_path);
    
    /**
     * Parses a .desktop file and extracts relevant metadata.
     * 
     * @param desktop_path Path to the .desktop file
     * @param info Output parameter for extracted metadata
     */
    static void parse_desktop_file(const std::string& desktop_path, CachedInfo& info);
    
    /**
     * Gets the cache directory path for a specific AppImage.
     * Uses SHA256 hash of path + mtime for uniqueness.
     * 
     * @param path Full path to the AppImage file
     * @param mtime File modification time
     * @return Cache directory path
     */
    static std::string get_cache_dir_for(const std::string& path, std::time_t mtime);
    
    /**
     * Clears all cached metadata.
     */
    void clear_cache();
    
private:
    std::unordered_map<std::string, CachedInfo> cache_;
    std::mutex cache_mutex_;
};

} // namespace Core
