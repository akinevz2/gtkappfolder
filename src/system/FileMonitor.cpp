#include "FileMonitor.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace System {

FileMonitor::FileMonitor() : main_monitor(nullptr) {
}

FileMonitor::~FileMonitor() {
    stop();
}

bool FileMonitor::start(const std::string& path, ChangeCallback callback, bool recursive) {
    stop(); // Stop any existing monitoring
    
    callback_ = callback;
    
    GFile* dir = g_file_new_for_path(path.c_str());
    main_monitor = g_file_monitor_directory(dir, G_FILE_MONITOR_NONE, nullptr, nullptr);
    g_object_unref(dir);
    
    if (!main_monitor) {
        std::cerr << "Failed to create file monitor for: " << path << std::endl;
        return false;
    }
    
    g_signal_connect(main_monitor, "changed", G_CALLBACK(on_dir_changed_internal), this);
    
    if (recursive) {
        monitor_subdirectories(path);
    }
    
    return true;
}

void FileMonitor::stop() {
    if (main_monitor) {
        g_file_monitor_cancel(main_monitor);
        g_object_unref(main_monitor);
        main_monitor = nullptr;
    }
    
    for (auto* monitor : sub_monitors) {
        if (monitor) {
            g_file_monitor_cancel(monitor);
            g_object_unref(monitor);
        }
    }
    sub_monitors.clear();
}

bool FileMonitor::is_monitoring() const {
    return main_monitor != nullptr;
}

void FileMonitor::monitor_subdirectories(const std::string& path) {
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                GFile* subdir = g_file_new_for_path(entry.path().c_str());
                GFileMonitor* sub_monitor = g_file_monitor_directory(subdir, G_FILE_MONITOR_NONE, nullptr, nullptr);
                g_object_unref(subdir);
                
                if (sub_monitor) {
                    g_signal_connect(sub_monitor, "changed", G_CALLBACK(on_dir_changed_internal), this);
                    sub_monitors.push_back(sub_monitor);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error monitoring subdirectories: " << e.what() << std::endl;
    }
}

void FileMonitor::on_dir_changed_internal(GFileMonitor* monitor, GFile* file,
                                          GFile* other_file, GFileMonitorEvent event_type,
                                          gpointer user_data) {
    FileMonitor* self = static_cast<FileMonitor*>(user_data);
    
    if (!self->callback_) {
        return;
    }
    
    char* file_path = g_file_get_path(file);
    char* other_file_path = other_file ? g_file_get_path(other_file) : nullptr;
    
    std::string path1 = file_path ? file_path : "";
    std::string path2 = other_file_path ? other_file_path : "";
    
    self->callback_(path1, path2, event_type);
    
    g_free(file_path);
    g_free(other_file_path);
}

} // namespace System
