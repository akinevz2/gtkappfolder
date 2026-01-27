#include "AppImage.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace Core {

std::vector<std::string> AppImageScanner::scan_directory(const std::string& path, bool recursive) {
    std::vector<std::string> results;
    
    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Invalid directory: " << path << std::endl;
            return results;
        }
        
        if (recursive) {
            scan_recursive(path, results);
        } else {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file() && is_appimage(entry.path().string())) {
                    results.push_back(entry.path().string());
                }
            }
        }
        
        // Sort alphabetically
        std::sort(results.begin(), results.end());
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    
    return results;
}

void AppImageScanner::scan_recursive(const std::string& path, std::vector<std::string>& results) {
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                scan_recursive(entry.path().string(), results);
            } else if (entry.is_regular_file() && is_appimage(entry.path().string())) {
                results.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error in recursive scan: " << e.what() << std::endl;
    }
}

std::map<std::string, std::vector<std::string>> AppImageScanner::group_by_directory(
    const std::vector<std::string>& appimages) {
    
    std::map<std::string, std::vector<std::string>> grouped;
    
    for (const auto& path : appimages) {
        fs::path p(path);
        std::string parent_dir = p.parent_path().string();
        grouped[parent_dir].push_back(path);
    }
    
    return grouped;
}

bool AppImageScanner::is_appimage(const std::string& filename) {
    if (filename.length() <= 9) {
        return false;
    }
    
    std::string ext = filename.substr(filename.length() - 9);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".appimage";
}

} // namespace Core
