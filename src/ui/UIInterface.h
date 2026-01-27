#pragma once

#include <gtk/gtk.h>
#include <string>
#include <functional>

namespace UI {

/**
 * Abstract interface for GTK UI implementations.
 * Hides GTK3/GTK4 differences from AppImageBrowser.
 */
class UIInterface {
public:
    virtual ~UIInterface() = default;
    
    // Lifecycle
    virtual void initialize(int argc, char* argv[]) = 0;
    virtual void create_window(const std::string& title, int width, int height) = 0;
    virtual void show_and_run() = 0;
    virtual void quit() = 0;
    
    // Widget creation
    virtual void create_path_bar(const std::string& initial_path) = 0;
    virtual void create_content_area() = 0;
    virtual void create_status_bar(const std::string& initial_text) = 0;
    
    // Path entry management
    virtual void setup_path_completion() = 0;
    virtual void update_path_completion(const std::string& current_text) = 0;
    virtual std::string get_path_entry_text() const = 0;
    virtual void set_path_entry_text(const std::string& text) = 0;
    
    // Content population
    virtual void clear_content() = 0;
    virtual void add_appimage_tile(const std::string& path, const std::string& filename,
                                   const std::string& icon_path) = 0;
    virtual void show_empty_message(const std::string& message) = 0;
    virtual void refresh_content() = 0;
    
    // Callbacks - must be set by user
    using PathChangedCallback = std::function<void(const std::string&)>;
    using RefreshCallback = std::function<void()>;
    using AppImageClickCallback = std::function<void(const std::string&)>;
    
    virtual void set_path_changed_callback(PathChangedCallback callback) = 0;
    virtual void set_refresh_callback(RefreshCallback callback) = 0;
    virtual void set_appimage_click_callback(AppImageClickCallback callback) = 0;
    
    // Access to raw window (for compatibility)
    virtual GtkWidget* get_window() = 0;
};

/**
 * Factory to create the appropriate UI implementation.
 */
UIInterface* create_ui();

} // namespace UI
