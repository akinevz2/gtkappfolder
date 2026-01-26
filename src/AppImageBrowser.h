#ifndef APPIMAGEBROWSER_H
#define APPIMAGEBROWSER_H

#include <gtk/gtk.h>
#include <string>
#include <vector>

class AppImageBrowser {
public:
    AppImageBrowser();
    ~AppImageBrowser();
    
    void run(int argc, char* argv[]);
    
private:
    GtkWidget* window;
    GtkWidget* scrolled_window;
    GtkWidget* list_box;
    GtkWidget* path_entry;
    GtkWidget* refresh_button;
    
    std::string current_directory;
    std::vector<std::string> appimage_files;
    
    // GTK callbacks
    static void on_appimage_clicked(GtkListBox* list_box, GtkListBoxRow* row, gpointer user_data);
    static void on_refresh_clicked(GtkButton* button, gpointer user_data);
    static void on_path_changed(GtkEntry* entry, gpointer user_data);
    static void on_destroy(GtkWidget* widget, gpointer user_data);
    
    // Helper methods
    void create_ui();
    void scan_directory(const std::string& path);
    void populate_list();
    void launch_appimage(const std::string& path);
    std::string get_current_directory();
};

#endif // APPIMAGEBROWSER_H
