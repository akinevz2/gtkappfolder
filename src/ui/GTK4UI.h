#pragma once

#include "UIInterface.h"
#include <adwaita.h>
#include <vector>

namespace UI {

/**
 * GTK4 with libadwaita implementation of the UI interface.
 */
class GTK4UI : public UIInterface {
public:
    GTK4UI();
    ~GTK4UI() override;
    
    void initialize(int argc, char* argv[]) override;
    void create_window(const std::string& title, int width, int height) override;
    void show_and_run() override;
    void quit() override;
    
    void create_path_bar(const std::string& initial_path) override;
    void create_content_area() override;
    void create_status_bar(const std::string& initial_text) override;
    
    void setup_path_completion() override;
    void update_path_completion(const std::string& current_text) override;
    std::string get_path_entry_text() const override;
    void set_path_entry_text(const std::string& text) override;
    
    void clear_content() override;
    void add_appimage_tile(const std::string& path, const std::string& filename,
                          const std::string& icon_path) override;
    void show_empty_message(const std::string& message) override;
    void refresh_content() override;
    
    void set_path_changed_callback(PathChangedCallback callback) override;
    void set_refresh_callback(RefreshCallback callback) override;
    void set_appimage_click_callback(AppImageClickCallback callback) override;
    
    GtkWidget* get_window() override { return GTK_WIDGET(window); }
    
private:
    GtkApplication* app;
    AdwApplicationWindow* window;
    GtkWidget* main_box;
    GtkWidget* path_entry;
    GtkWidget* refresh_button;
    GtkWidget* scrolled_window;
    GtkWidget* flow_box;
    GtkWidget* status_label;
    
    GtkEntryCompletion* path_completion;
    GtkListStore* completion_model;
    
    // Store window parameters for creation after app activation
    std::string window_title;
    int window_width;
    int window_height;
    std::string initial_path;
    std::string initial_status_text;
    bool window_created;
    bool path_completion_setup_deferred;
    
    // Deferred content structure
    struct DeferredTile {
        std::string path;
        std::string filename;
        std::string icon_path;
    };
    std::vector<DeferredTile> deferred_tiles;
    bool has_deferred_empty_message;
    std::string deferred_empty_message;
    
    PathChangedCallback on_path_changed;
    RefreshCallback on_refresh;
    AppImageClickCallback on_appimage_click;
    
    // GTK4 static callbacks
    static gboolean on_close_request_cb(GtkWidget* widget, gpointer user_data);
    static void on_app_activate_cb(GtkApplication* app, gpointer user_data);
    static void on_path_activate_cb(GtkEntry* entry, gpointer user_data);
    
    // Helper method for deferred window creation
    void create_window_deferred();
    void process_deferred_content();
    static void on_refresh_clicked_cb(GtkButton* button, gpointer user_data);
    static void on_appimage_clicked_cb(GtkButton* button, gpointer user_data);
    static void on_path_entry_changed_cb(GtkEditable* editable, gpointer user_data);
    static void on_path_click_cb(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data);
    static gboolean on_completion_match_cb(GtkEntryCompletion* completion, 
                                           const gchar* key, GtkTreeIter* iter, 
                                           gpointer user_data);
    static gboolean on_completion_match_selected_cb(GtkEntryCompletion* completion,
                                                    GtkTreeModel* model,
                                                    GtkTreeIter* iter,
                                                    gpointer user_data);
};

} // namespace UI
