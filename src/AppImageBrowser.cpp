#include "AppImageBrowser.h"
#include "ui/UIInterface.h"
#include <filesystem>
#include <iostream>
#include <unistd.h>

namespace fs = std::filesystem;

AppImageBrowser::AppImageBrowser()
    : preferences(new System::Preferences("com.github.gtkappfolder")),
      ui(nullptr) {
    current_directory = get_current_directory();
}

AppImageBrowser::~AppImageBrowser() {
    delete preferences;
    delete ui;
}

std::string AppImageBrowser::get_current_directory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return ".";
}

void AppImageBrowser::run(int argc, char* argv[]) {
    // Create UI implementation (GTK3 or GTK4 based on compile-time flag)
    ui = UI::create_ui();
    
    // Initialize GTK
    ui->initialize(argc, argv);
    
    // Create window and UI components
    ui->create_window("AppImage Browser", 600, 400);
    ui->create_path_bar(current_directory);
    ui->create_content_area();
    ui->create_status_bar("Click on an AppImage to launch it");
    
    // Setup autocomplete
    ui->setup_path_completion();
    
    // Set up callbacks
    ui->set_path_changed_callback([this](const std::string& path) {
        this->on_path_changed(path);
    });
    
    ui->set_refresh_callback([this]() {
        this->on_refresh();
    });
    
    ui->set_appimage_click_callback([this](const std::string& path) {
        this->on_appimage_click(path);
    });
    
    // Initial scan and populate
    scan_directory(current_directory);
    populate_list();
    
    // Show and run
    ui->show_and_run();
}

void AppImageBrowser::scan_directory(const std::string& path) {
    appimage_files = Core::AppImageScanner::scan_directory(path, false);
}

void AppImageBrowser::populate_list() {
    ui->clear_content();
    
    if (appimage_files.empty()) {
        ui->show_empty_message("No AppImage files found in this directory");
    } else {
        for (const auto& file : appimage_files) {
            fs::path p(file);
            std::string filename = p.filename().string();
            ui->add_appimage_tile(file, filename, "");
        }
    }
    
    ui->refresh_content();
}

void AppImageBrowser::launch_appimage(const std::string& path) {
    System::ProcessManager::launch_appimage(path);
}

// UI event handlers
void AppImageBrowser::on_path_changed(const std::string& new_path) {
    current_directory = new_path;
    scan_directory(current_directory);
    populate_list();
}

void AppImageBrowser::on_refresh() {
    std::string path = ui->get_path_entry_text();
    current_directory = path;
    scan_directory(current_directory);
    populate_list();
}

void AppImageBrowser::on_appimage_click(const std::string& path) {
    launch_appimage(path);
}