#pragma once

#include <string>
#include <vector>
#include <map>

// Core and System modules
#include "core/AppImage.h"
#include "core/AppImageMetadata.h"
#include "system/ProcessManager.h"
#include "system/FileMonitor.h"
#include "system/Preferences.h"

// UI abstraction - NO GTK includes needed here
namespace UI {
    class UIInterface;
}

class AppImageBrowser {
public:
    AppImageBrowser();
    ~AppImageBrowser();
    
    void run(int argc, char* argv[]);
    
private:
    // Core and System modules
    Core::AppImageMetadata metadata_manager;
    System::Preferences* preferences;
    
    // UI abstraction
    UI::UIInterface* ui;
    
    // Application state
    std::string current_directory;
    std::vector<std::string> appimage_files;
    std::map<std::string, std::vector<std::string>> grouped_files;
    
    // Helper methods
    void scan_directory(const std::string& path);
    void populate_list();
    void launch_appimage(const std::string& path);
    std::string get_current_directory();
    
    // UI event handlers
    void on_path_changed(const std::string& new_path);
    void on_refresh();
    void on_appimage_click(const std::string& path);
};
