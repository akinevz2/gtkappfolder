#include "AppImageMetadata.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <openssl/sha.h>

namespace fs = std::filesystem;

namespace Core {

const CachedInfo* AppImageMetadata::get_cached_info(const std::string& path) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        return &it->second;
    }
    return nullptr;
}

void AppImageMetadata::cache_info(const std::string& path, const CachedInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[path] = info;
}

void AppImageMetadata::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

std::string AppImageMetadata::get_cache_dir_for(const std::string& path, std::time_t mtime) {
    // Create a unique hash based on path and modification time
    std::string input = path + std::to_string(mtime);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    std::string cache_home = std::getenv("XDG_CACHE_HOME") 
        ? std::getenv("XDG_CACHE_HOME") 
        : (std::string(std::getenv("HOME")) + "/.cache");
    
    return cache_home + "/gtkappfolder/" + ss.str();
}

bool AppImageMetadata::find_desktop_file(const std::string& root, std::string& out_path) {
    try {
        // Common locations for .desktop files in AppImages
        std::vector<std::string> search_paths = {
            root + "/usr/share/applications",
            root + "/share/applications",
            root
        };
        
        for (const auto& search_path : search_paths) {
            if (!fs::exists(search_path)) continue;
            
            for (const auto& entry : fs::recursive_directory_iterator(search_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                    out_path = entry.path().string();
                    return true;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error searching for desktop file: " << e.what() << std::endl;
    }
    
    return false;
}

void AppImageMetadata::parse_desktop_file(const std::string& desktop_path, CachedInfo& info) {
    std::ifstream file(desktop_path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    bool in_desktop_entry = false;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line == "[Desktop Entry]") {
            in_desktop_entry = true;
            continue;
        } else if (line.empty() || line[0] == '#') {
            continue;
        } else if (line[0] == '[') {
            in_desktop_entry = false;
            continue;
        }
        
        if (!in_desktop_entry) continue;
        
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        if (key == "Name") {
            info.name = value;
        } else if (key == "Comment") {
            info.comment = value;
        } else if (key == "Icon") {
            // Icon path will be resolved separately
            info.icon_path = value;
        }
    }
    
    file.close();
}

std::string AppImageMetadata::extract_appimage_icon(const std::string& path) {
    try {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            return "";
        }
        
        std::time_t mtime = st.st_mtime;
        std::string cache_dir = get_cache_dir_for(path, mtime);
        
        // Check if already cached
        if (fs::exists(cache_dir)) {
            // Find icon in cache
            for (const auto& entry : fs::directory_iterator(cache_dir)) {
                std::string filename = entry.path().filename().string();
                if (filename.find("icon") != std::string::npos || 
                    entry.path().extension() == ".png" || 
                    entry.path().extension() == ".svg") {
                    return entry.path().string();
                }
            }
        }
        
        // Create cache directory
        fs::create_directories(cache_dir);
        
        // Mount AppImage
        std::string mount_dir = cache_dir + "/squashfs-root";
        
        // Extract using --appimage-extract
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            chdir(cache_dir.c_str());
            execl(path.c_str(), path.c_str(), "--appimage-extract", nullptr);
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                // Look for .DirIcon or parse .desktop file
                std::string diricon_path = mount_dir + "/.DirIcon";
                if (fs::exists(diricon_path)) {
                    std::string icon_dest = cache_dir + "/icon" + fs::path(diricon_path).extension().string();
                    fs::copy_file(diricon_path, icon_dest, fs::copy_options::overwrite_existing);
                    return icon_dest;
                }
                
                // Parse .desktop file for Icon=
                std::string desktop_path;
                if (find_desktop_file(mount_dir, desktop_path)) {
                    CachedInfo temp_info;
                    parse_desktop_file(desktop_path, temp_info);
                    
                    if (!temp_info.icon_path.empty()) {
                        // Try to find icon file
                        std::vector<std::string> icon_extensions = {".png", ".svg", ".xpm"};
                        for (const auto& ext : icon_extensions) {
                            std::string icon_file = mount_dir + "/usr/share/icons/hicolor/256x256/apps/" + temp_info.icon_path + ext;
                            if (fs::exists(icon_file)) {
                                std::string icon_dest = cache_dir + "/icon" + ext;
                                fs::copy_file(icon_file, icon_dest, fs::copy_options::overwrite_existing);
                                return icon_dest;
                            }
                        }
                    }
                }
                
                // Clean up extracted files
                fs::remove_all(mount_dir);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error extracting icon: " << e.what() << std::endl;
    }
    
    return "";
}

} // namespace Core
