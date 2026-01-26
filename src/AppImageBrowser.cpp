#include "AppImageBrowser.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

AppImageBrowser::AppImageBrowser() 
    : window(nullptr), scrolled_window(nullptr), flow_box(nullptr),
      path_entry(nullptr), refresh_button(nullptr) {
    current_directory = get_current_directory();
}

AppImageBrowser::~AppImageBrowser() {
    // GTK widgets are freed automatically when the window is destroyed
}

std::string AppImageBrowser::get_current_directory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return ".";
}

void AppImageBrowser::run(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    create_ui();
    scan_directory(current_directory);
    populate_list();
    
    gtk_widget_show_all(window);
    gtk_main();
}

void AppImageBrowser::create_ui() {
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "AppImage Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), this);
    
    // Create main vertical box
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Create horizontal box for path entry and refresh button
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    // Path label
    GtkWidget* label = gtk_label_new("Directory:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    // Path entry
    path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(path_entry), current_directory.c_str());
    gtk_box_pack_start(GTK_BOX(hbox), path_entry, TRUE, TRUE, 0);
    g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_changed), this);
    
    // Refresh button
    refresh_button = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(hbox), refresh_button, FALSE, FALSE, 0);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), this);
    
    // Create scrolled window
    scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Create flow box for grid layout
    flow_box = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flow_box), TRUE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_container_add(GTK_CONTAINER(scrolled_window), flow_box);
    
    // Status bar
    GtkWidget* statusbar = gtk_label_new("Click on an AppImage to launch it");
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
}

void AppImageBrowser::scan_directory(const std::string& path) {
    appimage_files.clear();
    
    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Invalid directory: " << path << std::endl;
            return;
        }
        
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // Check if file ends with .AppImage (case-insensitive)
                if (filename.length() > 9) {
                    std::string ext = filename.substr(filename.length() - 9);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".appimage") {
                        appimage_files.push_back(entry.path().string());
                    }
                }
            }
        }
        
        // Sort alphabetically
        std::sort(appimage_files.begin(), appimage_files.end());
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

void AppImageBrowser::populate_list() {
    // Clear existing items
    GList* children = gtk_container_get_children(GTK_CONTAINER(flow_box));
    for (GList* iter = children; iter != nullptr; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add AppImage files to grid
    if (appimage_files.empty()) {
        GtkWidget* label = gtk_label_new("No AppImage files found in this directory");
        gtk_container_add(GTK_CONTAINER(flow_box), label);
    } else {
        for (const auto& file : appimage_files) {
            // Create button for each AppImage
            GtkWidget* button = gtk_button_new();
            gtk_widget_set_size_request(button, 200, 200);
            
            // Create vertical box inside button
            GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_add(GTK_CONTAINER(button), vbox);
            
            // Add spacer at top
            GtkWidget* top_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_pack_start(GTK_BOX(vbox), top_spacer, TRUE, TRUE, 0);
            
            // Add icon (large application icon)
            GtkWidget* icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 96);
            gtk_box_pack_start(GTK_BOX(vbox), icon, FALSE, FALSE, 5);
            
            // Extract filename from path
            fs::path p(file);
            std::string filename = p.filename().string();
            
            // Create label with word wrapping
            GtkWidget* label = gtk_label_new(filename.c_str());
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
            gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
            gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
            gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
            
            // Add spacer at bottom
            GtkWidget* bottom_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_pack_start(GTK_BOX(vbox), bottom_spacer, TRUE, TRUE, 0);
            
            // Store full path as data
            g_object_set_data_full(G_OBJECT(button), "appimage_path", 
                                   g_strdup(file.c_str()), g_free);
            
            // Connect click signal
            g_signal_connect(button, "clicked", G_CALLBACK(on_appimage_clicked), this);
            
            gtk_container_add(GTK_CONTAINER(flow_box), button);
        }
    }
    
    gtk_widget_show_all(flow_box);
}

void AppImageBrowser::launch_appimage(const std::string& path) {
    std::cout << "Launching: " << path << std::endl;
    
    // Check if file is executable
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (!(st.st_mode & S_IXUSR)) {
            // Make it executable
            chmod(path.c_str(), st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
        }
    }
    
    // Launch the AppImage in background
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl(path.c_str(), path.c_str(), nullptr);
        // If execl returns, there was an error
        std::cerr << "Failed to launch: " << path << std::endl;
        exit(1);
    } else if (pid < 0) {
        std::cerr << "Failed to fork process" << std::endl;
    }
}

// GTK Callbacks
void AppImageBrowser::on_appimage_clicked(GtkButton* button, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    
    const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(button), "appimage_path"));
    if (path != nullptr) {
        browser->launch_appimage(path);
    }
}

void AppImageBrowser::on_refresh_clicked(GtkButton* button, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    
    const char* path = gtk_entry_get_text(GTK_ENTRY(browser->path_entry));
    browser->current_directory = path;
    browser->scan_directory(browser->current_directory);
    browser->populate_list();
}

void AppImageBrowser::on_path_changed(GtkEntry* entry, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    
    const char* path = gtk_entry_get_text(entry);
    browser->current_directory = path;
    browser->scan_directory(browser->current_directory);
    browser->populate_list();
}

void AppImageBrowser::on_destroy(GtkWidget* widget, gpointer user_data) {
    gtk_main_quit();
}
