#pragma once

#include <gio/gio.h>
#include <string>
#include <vector>
#include <functional>

namespace System {

/**
 * Wraps GFileMonitor functionality for watching directory changes.
 * Provides a cleaner interface for file system monitoring.
 */
class FileMonitor {
public:
    /**
     * Callback type for directory change events.
     * Parameters: file path, other file path (for renames/moves), event type
     */
    using ChangeCallback = std::function<void(const std::string&, const std::string&, GFileMonitorEvent)>;
    
    FileMonitor();
    ~FileMonitor();
    
    /**
     * Starts monitoring a directory for changes.
     * 
     * @param path Directory path to monitor
     * @param callback Function to call when changes are detected
     * @param recursive If true, also monitors subdirectories
     * @return true if monitoring started successfully, false otherwise
     */
    bool start(const std::string& path, ChangeCallback callback, bool recursive = false);
    
    /**
     * Stops all monitoring.
     */
    void stop();
    
    /**
     * Checks if currently monitoring.
     * 
     * @return true if actively monitoring, false otherwise
     */
    bool is_monitoring() const;
    
private:
    GFileMonitor* main_monitor;
    std::vector<GFileMonitor*> sub_monitors;
    ChangeCallback callback_;
    
    /**
     * Internal GFileMonitor callback.
     */
    static void on_dir_changed_internal(GFileMonitor* monitor, GFile* file, 
                                       GFile* other_file, GFileMonitorEvent event_type, 
                                       gpointer user_data);
    
    /**
     * Monitors subdirectories recursively.
     */
    void monitor_subdirectories(const std::string& path);
};

} // namespace System
