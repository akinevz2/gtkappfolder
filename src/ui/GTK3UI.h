#pragma once

#include "UIInterface.h"
#include <vector>

namespace UI {

/**
 * GTK3 implementation of the UI interface.
 */
class GTK3UI : public UIInterface {
public:
    GTK3UI();
    ~GTK3UI() override;
    
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
    
    GtkWidget* get_window() override { return window; }
    
private:
    GtkWidget* window;
    GtkWidget* main_vbox;
    GtkWidget* path_entry;
    GtkWidget* refresh_button;
    GtkWidget* scrolled_window;
    GtkWidget* flow_box;
    GtkWidget* status_label;
    
    GtkEntryCompletion* path_completion;
    GtkListStore* completion_model;
    
    PathChangedCallback on_path_changed;
    RefreshCallback on_refresh;
    AppImageClickCallback on_appimage_click;
    
    // GTK3 static callbacks
    static void on_destroy_cb(GtkWidget* widget, gpointer user_data);
    static void on_path_activate_cb(GtkEntry* entry, gpointer user_data);
    static void on_refresh_clicked_cb(GtkButton* button, gpointer user_data);
    static void on_appimage_clicked_cb(GtkButton* button, gpointer user_data);
    static void on_path_entry_changed_cb(GtkEditable* editable, gpointer user_data);
    static gboolean on_path_button_press_cb(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
    static gboolean on_completion_match_cb(GtkEntryCompletion* completion, 
                                           const gchar* key, GtkTreeIter* iter, 
                                           gpointer user_data);
    static gboolean on_completion_match_selected_cb(GtkEntryCompletion* completion,
                                                    GtkTreeModel* model,
                                                    GtkTreeIter* iter,
                                                    gpointer user_data);
};

} // namespace UI
