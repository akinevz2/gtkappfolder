#ifndef APPIMAGEBROWSER_H
#define APPIMAGEBROWSER_H

#include <gtk/gtk.h>
#include <adwaita.h>
#include <gio/gio.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <ctime>
#include <mutex>

class AppImageBrowser {
public:
    AppImageBrowser();
    ~AppImageBrowser();
    
    void run(int argc, char* argv[]);
    
private:
    struct CachedInfo {
        std::string name;
        std::string comment;
        std::string icon_path;
        std::time_t mtime{0};
    };
    GtkWidget* window;
    GtkWidget* scrolled_window;
    GtkWidget* content_stack;
    GtkWidget* flow_box; // legacy, unused in grouped view
    GtkWidget* groups_box; // container holding multiple group sections
    GtkWidget* empty_view;
    GtkWidget* path_entry;
    GtkWidget* refresh_button;
    GtkWidget* open_button;
    GtkWidget* install_button;
    GtkWidget* status_label;
    GtkWidget* autoclose_switch;
    bool autoclose_enabled{false};
    GSettings* settings{nullptr};
    std::unordered_map<std::string, CachedInfo> cache_;
    std::mutex cache_mutex_;
    GFileMonitor* dir_monitor{nullptr};
    std::vector<GFileMonitor*> sub_monitors;
    
    std::string current_directory;
    std::vector<std::string> appimage_files;
    std::map<std::string, std::vector<std::string>> grouped_files;
    
    // GTK callbacks
    static void on_appimage_clicked(GtkButton* button, gpointer user_data);
    static void on_refresh_clicked(GtkButton* button, gpointer user_data);
    static void on_path_changed(GtkEntry* entry, gpointer user_data);
    static gboolean on_destroy(GtkWidget* widget, gpointer user_data);
    static void on_open_folder(GtkButton* button, gpointer user_data);
    static void on_install_requirements(GtkButton* button, gpointer user_data);
    static void on_item_clicked(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data);
    static void on_menu_open_button_clicked(GtkButton* button, gpointer user_data);
    static void on_menu_show_in_folder_button_clicked(GtkButton* button, gpointer user_data);
    static void on_menu_properties_button_clicked(GtkButton* button, gpointer user_data);
    static void on_dir_changed(GFileMonitor* monitor, GFile* file, GFile* other_file, GFileMonitorEvent event_type, gpointer user_data);
    static gboolean on_drop(GtkDropTarget* target, const GValue* value, double x, double y, gpointer user_data);
    static gboolean on_autoclose_switch(GtkSwitch* widget, gboolean state, gpointer user_data);
    static gpointer metadata_thread_func(gpointer data);
    
    // Helper methods
    void create_ui(GtkApplication* app);
    void scan_directory(const std::string& path);
    void populate_list();
    void launch_appimage(const std::string& path);
    std::string get_current_directory();
    void open_folder_dialog();
    bool has_fuse2();
    void install_requirements();
    void set_status(const std::string& text);
    void start_dir_monitor(const std::string& path);
    void stop_dir_monitor();
    void start_subdir_monitors(const std::string& path);
    void stop_subdir_monitors();
    void init_settings();
    std::string get_exe_dir();
    GtkWidget* build_context_menu(const std::string& path);
    GtkWidget* build_appimage_tile(const std::string& file);
    std::string extract_appimage_icon(const std::string& path);
    void load_metadata_async(const std::string& path, GtkWidget* button);
    std::string get_cache_dir_for(const std::string& path, std::time_t mtime);
    bool find_desktop_file(const std::string& root, std::string& out_path);
    void parse_desktop_file(const std::string& desktop_path, CachedInfo& info);
};

#endif // APPIMAGEBROWSER_H
