#include "AppImageBrowser.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <unordered_map>

namespace fs = std::filesystem;

AppImageBrowser::AppImageBrowser() 
        : window(nullptr), scrolled_window(nullptr), flow_box(nullptr),
            path_entry(nullptr), refresh_button(nullptr),
            open_button(nullptr), install_button(nullptr), status_label(nullptr),
            dir_monitor(nullptr) {
        const char* home = g_get_home_dir();
        current_directory = home ? std::string(home) : get_current_directory();
        init_settings();
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
    // Make the app non-unique so a stale or running instance on the session bus
    // doesn't cause this invocation to exit immediately.
    GtkApplication* app = gtk_application_new(
        "com.github.gtkappfolder",
        static_cast<GApplicationFlags>(G_APPLICATION_DEFAULT_FLAGS | G_APPLICATION_NON_UNIQUE));
    g_signal_connect(app, "activate", G_CALLBACK(+[](GtkApplication* app, gpointer user_data) {
        AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
        browser->create_ui(app);
        browser->scan_directory(browser->current_directory);
        browser->populate_list();
        browser->start_dir_monitor(browser->current_directory);
        // Add window to application to keep it alive (critical for NON_UNIQUE flag)
        gtk_application_add_window(app, GTK_WINDOW(browser->window));
        gtk_window_present(GTK_WINDOW(browser->window));
    }), this);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    std::exit(status);
}

void AppImageBrowser::create_ui(GtkApplication* app) {
    // Create main window with AdwApplicationWindow bound to the application
    window = adw_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "AppImage Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);
    g_signal_connect(window, "close-request", G_CALLBACK(on_destroy), this);
    
    // Create a box for the content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), vbox);
    
    // Header bar with AdwHeaderBar inside a box
    GtkWidget* header_bar = adw_header_bar_new();
    gtk_box_prepend(GTK_BOX(vbox), header_bar);
    
    // Autoclose switch on the header bar
    GtkWidget* switch_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* switch_label = gtk_label_new("Autoclose");
    autoclose_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(autoclose_switch), autoclose_enabled);
    g_signal_connect(autoclose_switch, "state-set", G_CALLBACK(on_autoclose_switch), this);
    gtk_box_append(GTK_BOX(switch_box), switch_label);
    gtk_box_append(GTK_BOX(switch_box), autoclose_switch);
    adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar), switch_box);
    
    // Controls row: path entry + buttons
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(vbox), hbox);

    // Path entry
    path_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(path_entry), "Folder path");
    gtk_editable_set_text(GTK_EDITABLE(path_entry), current_directory.c_str());
    gtk_widget_set_hexpand(path_entry, TRUE);
    gtk_box_append(GTK_BOX(hbox), path_entry);
    g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_changed), this);

    // Open folder button
    open_button = gtk_button_new_with_label("Open Folder…");
    gtk_box_append(GTK_BOX(hbox), open_button);
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_folder), this);

    // Refresh button
    refresh_button = gtk_button_new_with_label("Refresh");
    gtk_box_append(GTK_BOX(hbox), refresh_button);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), this);

    // Install requirements button in header bar
    install_button = gtk_button_new_with_label("Install Requirements");
    adw_header_bar_pack_end(ADW_HEADER_BAR(header_bar), install_button);
    g_signal_connect(install_button, "clicked", G_CALLBACK(on_install_requirements), this);
    
    // Create scrolled window
    GtkWidget* scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(vbox), scrolled_window);

    // Stack to toggle between grid and empty state
    content_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(content_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), content_stack);
    
    // Groups container for grid sections
    groups_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(groups_box, 8);
    gtk_widget_set_margin_bottom(groups_box, 8);
    gtk_widget_set_margin_start(groups_box, 8);
    gtk_widget_set_margin_end(groups_box, 8);
    gtk_stack_add_named(GTK_STACK(content_stack), groups_box, "grid");

    // Flat grid as alternative view
    flow_box = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flow_box), TRUE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_widget_set_margin_top(flow_box, 8);
    gtk_widget_set_margin_bottom(flow_box, 8);
    gtk_widget_set_margin_start(flow_box, 8);
    gtk_widget_set_margin_end(flow_box, 8);
    gtk_stack_add_named(GTK_STACK(content_stack), flow_box, "flat");

    // Empty view, top-aligned
    empty_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_valign(empty_view, GTK_ALIGN_START);
    gtk_widget_set_halign(empty_view, GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(empty_view, 20);
    gtk_widget_set_margin_start(empty_view, 20);
    GtkWidget* empty_label = gtk_label_new("No AppImage files found in this directory");
    gtk_label_set_xalign(GTK_LABEL(empty_label), 0.0);
    gtk_box_append(GTK_BOX(empty_view), empty_label);
    gtk_stack_add_named(GTK_STACK(content_stack), empty_view, "empty");
    
    // Status bar
    status_label = gtk_label_new("Click on an AppImage to launch it");
    gtk_widget_set_margin_top(status_label, 8);
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_append(GTK_BOX(vbox), status_label);
    
    // Enable drag-and-drop for URI lists and files (add AppImages by dropping)
    GtkDropTarget* drop_target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    // Accept both string URIs and GdkFileList
    GType types[] = { G_TYPE_STRING };
    gtk_drop_target_set_gtypes(drop_target, types, 1);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_drop), this);
    gtk_widget_add_controller(window, GTK_EVENT_CONTROLLER(drop_target));
    
    gtk_widget_set_visible(vbox, TRUE);
}

void AppImageBrowser::scan_directory(const std::string& path) {
    appimage_files.clear();
    grouped_files.clear();
    
    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Invalid directory: " << path << std::endl;
            return;
        }
        
        // Helper lambda to test extension
        auto is_appimage = [](const std::string& name) {
            if (name.size() < 9) return false;
            std::string ext = name.substr(name.size() - 9);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".appimage";
        };

        // Group name for base directory
        std::string base_group = fs::path(path).filename().string();
        if (base_group.empty()) base_group = path; // for home '/home/user'

        // First, files directly in base path
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (is_appimage(filename)) {
                    grouped_files[base_group].push_back(entry.path().string());
                }
            }
        }

        // Then, one level of subdirectories
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                std::string group = entry.path().filename().string();
                std::vector<std::string> items;
                for (const auto& sub : fs::directory_iterator(entry.path())) {
                    if (sub.is_regular_file()) {
                        std::string fn = sub.path().filename().string();
                        if (is_appimage(fn)) items.push_back(sub.path().string());
                    }
                }
                if (!items.empty()) {
                    std::sort(items.begin(), items.end());
                    grouped_files[group] = std::move(items);
                }
            }
        }
        
        // Flatten to legacy list for counts
        for (const auto& kv : grouped_files) {
            appimage_files.insert(appimage_files.end(), kv.second.begin(), kv.second.end());
        }
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

void AppImageBrowser::populate_list() {
    // Clear existing groups content
    {
        GtkWidget* child = gtk_widget_get_first_child(groups_box);
        while (child) {
            GtkWidget* next = gtk_widget_get_next_sibling(child);
            gtk_widget_unparent(child);
            child = next;
        }
    }
    
    // Add AppImage files to grid
    if (appimage_files.empty()) {
        gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "empty");
        set_status("No AppImage files found");
    } else {
        gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "grid");
        set_status(std::to_string(appimage_files.size()) + " AppImage(s) found");
        // Clear the flat grid too (so it doesn't accumulate)
        {
            GtkWidget* child = gtk_widget_get_first_child(flow_box);
            while (child) {
                GtkWidget* next = gtk_widget_get_next_sibling(child);
                gtk_widget_unparent(child);
                child = next;
            }
        }

        // Build sorted group list
        std::vector<std::pair<std::string, std::vector<std::string>>> groups;
        for (const auto& kv : grouped_files) groups.push_back(kv);
        std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

        for (const auto& kv : groups) {
            const std::string& group_name = kv.first;
            const auto& files = kv.second;
            // Group header with full directory path
            GtkWidget* header = gtk_label_new(nullptr);
            std::string full_path;
            if (group_name == fs::path(current_directory).filename().string()) {
                // Base directory
                full_path = current_directory;
            } else {
                // Subdirectory
                full_path = current_directory + "/" + group_name;
            }
            std::string markup = std::string("<b>") + group_name + "</b> (" + std::to_string(files.size()) + ")\n<small>" + full_path + "</small>";
            gtk_label_set_use_markup(GTK_LABEL(header), TRUE);
            gtk_label_set_markup(GTK_LABEL(header), markup.c_str());
            gtk_label_set_xalign(GTK_LABEL(header), 0.0);
            gtk_box_append(GTK_BOX(groups_box), header);
            // Group grid
            GtkWidget* grid = gtk_flow_box_new();
            gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(grid), 2);
            gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(grid), 2);
            gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(grid), GTK_SELECTION_NONE);
            gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(grid), TRUE);
            gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(grid), 10);
            gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(grid), 10);
            gtk_box_append(GTK_BOX(groups_box), grid);
            for (const auto& file : files) {
                GtkWidget* tile = build_appimage_tile(file);
                gtk_flow_box_append(GTK_FLOW_BOX(grid), tile);
                load_metadata_async(file, tile);
            }
        }
    }

    gtk_widget_set_visible(content_stack, TRUE);
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

    // Prefer GSubprocess for robust spawning
    GError* error = nullptr;
    GSubprocessLauncher* launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_NONE);

    // Check if FUSE2 is available - required for sandboxing
    if (!has_fuse2()) {
        set_status("FUSE2 is required to run AppImages. Click 'Install Requirements' to install it.");
        return;
    }

    // Launch AppImage with FUSE2 sandboxing
    set_status("Launching AppImage…");

    // Build argv
    const char* argvv[] = { path.c_str(), nullptr };
    GSubprocess* proc = g_subprocess_launcher_spawnv(launcher, argvv, &error);
    g_object_unref(launcher);

    if (!proc) {
        std::cerr << "Failed to launch: " << path << ": "
                  << (error ? error->message : "unknown error") << std::endl;
        if (error) g_error_free(error);
        set_status("Launch failed. Check dependencies.");
        return;
    }

    // Do not wait; just unref to let it run
    g_object_unref(proc);
    
    // If autoclose is enabled, schedule window close after 250ms
    if (autoclose_enabled) {
        g_timeout_add(250, [](gpointer user_data) -> gboolean {
            AppImageBrowser* self = static_cast<AppImageBrowser*>(user_data);
            gtk_window_close(GTK_WINDOW(self->window));
            return G_SOURCE_REMOVE;
        }, this);
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
    
    const char* path = gtk_editable_get_text(GTK_EDITABLE(browser->path_entry));
    browser->current_directory = path;
    if (browser->settings) g_settings_set_string(browser->settings, "last-folder", browser->current_directory.c_str());
    browser->scan_directory(browser->current_directory);
    browser->populate_list();
    browser->start_dir_monitor(browser->current_directory);
}

void AppImageBrowser::on_path_changed(GtkEntry* entry, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    
    const char* path = gtk_editable_get_text(GTK_EDITABLE(entry));
    browser->current_directory = path;
    if (browser->settings) g_settings_set_string(browser->settings, "last-folder", browser->current_directory.c_str());
    browser->scan_directory(browser->current_directory);
    browser->populate_list();
    browser->start_dir_monitor(browser->current_directory);
}

gboolean AppImageBrowser::on_destroy(GtkWidget* widget, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    if (browser) {
        browser->stop_dir_monitor();
        if (browser->settings) {
            g_object_unref(browser->settings);
            browser->settings = nullptr;
        }
    }
    // Returning FALSE allows GTK to destroy the window normally.
    return FALSE;
}

void AppImageBrowser::on_open_folder(GtkButton* /*button*/, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    browser->open_folder_dialog();
}

void AppImageBrowser::on_install_requirements(GtkButton* /*button*/, gpointer user_data) {
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    browser->install_requirements();
}

void AppImageBrowser::open_folder_dialog() {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Folder");
    gtk_file_dialog_set_accept_label(dialog, "Open");
    
    // Set initial folder if possible
    GFile* initial_folder = g_file_new_for_path(current_directory.c_str());
    gtk_file_dialog_set_initial_folder(dialog, initial_folder);
    g_object_unref(initial_folder);
    
    // Launch async dialog
    gtk_file_dialog_select_folder(
        dialog,
        GTK_WINDOW(window),
        nullptr,  // cancellable
        [](GObject* source, GAsyncResult* result, gpointer user_data) {
            AppImageBrowser* self = static_cast<AppImageBrowser*>(user_data);
            GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
            GError* error = nullptr;
            GFile* folder = gtk_file_dialog_select_folder_finish(dialog, result, &error);
            
            if (folder) {
                gchar* path = g_file_get_path(folder);
                if (path) {
                    self->current_directory = path;
                    gtk_editable_set_text(GTK_EDITABLE(self->path_entry), self->current_directory.c_str());
                    if (self->settings) g_settings_set_string(self->settings, "last-folder", self->current_directory.c_str());
                    g_free(path);
                    self->scan_directory(self->current_directory);
                    self->populate_list();
                    self->start_dir_monitor(self->current_directory);
                }
                g_object_unref(folder);
            } else if (error) {
                // User cancelled or error occurred
                g_error_free(error);
            }
            
            g_object_unref(dialog);
        },
        this
    );
}

bool AppImageBrowser::has_fuse2() {
    // Try ldconfig -p and search for libfuse.so.2
    GError* error = nullptr;
    GSubprocess* proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                                         &error,
                                         "ldconfig", "-p", nullptr);
    if (!proc) {
        if (error) g_error_free(error);
        return false;
    }
    gchar* out = nullptr;
    gchar* err = nullptr;
    if (!g_subprocess_communicate_utf8(proc, nullptr, nullptr, &out, &err, &error)) {
        if (error) g_error_free(error);
        g_object_unref(proc);
        return false;
    }
    bool found = false;
    if (out) {
        std::string s(out);
        found = (s.find("libfuse.so.2") != std::string::npos);
        g_free(out);
    }
    if (err) g_free(err);
    g_object_unref(proc);
    return found;
}

void AppImageBrowser::install_requirements() {
    // Check if FUSE2 is already installed
    if (has_fuse2()) {
        AdwAlertDialog* dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
            "FUSE2 Already Installed",
            "FUSE2 is already available on your system. AppImages will run with proper mounting and sandboxing."));
        adw_alert_dialog_add_response(dialog, "ok", "OK");
        adw_alert_dialog_set_default_response(dialog, "ok");
        adw_alert_dialog_set_close_response(dialog, "ok");
        adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
        return;
    }

    set_status("Checking and installing FUSE2 (admin required)…");

    // Determine package manager and build appropriate install command
    const char* pkg = nullptr;
    const char* install_cmd = nullptr;

    if (g_find_program_in_path("apt-get")) {
        pkg = "apt";
        install_cmd = "add-apt-repository universe && apt-get update && apt-get install -y libfuse2";
    } else if (g_find_program_in_path("dnf")) {
        pkg = "dnf";
        install_cmd = "dnf install -y fuse fuse-libs";
    } else if (g_find_program_in_path("yum")) {
        pkg = "yum";
        install_cmd = "dnf install -y fuse fuse-libs";
    } else if (g_find_program_in_path("pacman")) {
        pkg = "pacman";
        install_cmd = "pacman -Sy --noconfirm fuse2";
    } else if (g_find_program_in_path("zypper")) {
        pkg = "zypper";
        install_cmd = "zypper install -y fuse2";
    }

    if (!pkg || !install_cmd) {
        set_status("No supported package manager found.");
        return;
    }

    // Build pkexec command
    std::string cmd = std::string("sh -c \"") + install_cmd + "\"";
    GError* error = nullptr;
    GSubprocess* proc = g_subprocess_new(G_SUBPROCESS_FLAGS_NONE,
                                         &error,
                                         "pkexec", "sh", "-c", cmd.c_str(), nullptr);
    if (!proc) {
        std::cerr << "pkexec failed: " << (error ? error->message : "unknown") << std::endl;
        if (error) g_error_free(error);
        set_status("Failed to start installer.");
        return;
    }

    // We won't block UI; but we can try to wait a bit and update status later in a thread.
    // For simplicity, just fire-and-forget and inform the user.
    g_object_unref(proc);
    set_status(std::string("Installer launched via pkexec using ") + pkg + ".");
}

void AppImageBrowser::set_status(const std::string& text) {
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), text.c_str());
    }
}

void AppImageBrowser::start_dir_monitor(const std::string& path) {
    stop_dir_monitor();
    GFile* dir = g_file_new_for_path(path.c_str());
    GError* error = nullptr;
    dir_monitor = g_file_monitor_directory(dir, G_FILE_MONITOR_NONE, nullptr, &error);
    g_object_unref(dir);
    if (!dir_monitor) {
        if (error) g_error_free(error);
        return;
    }
    g_signal_connect(dir_monitor, "changed", G_CALLBACK(on_dir_changed), this);
    start_subdir_monitors(path);
}

void AppImageBrowser::stop_dir_monitor() {
    if (dir_monitor) {
        g_object_unref(dir_monitor);
        dir_monitor = nullptr;
    }
    stop_subdir_monitors();
}

void AppImageBrowser::on_dir_changed(GFileMonitor* /*monitor*/, GFile* /*file*/, GFile* /*other_file*/, GFileMonitorEvent event_type, gpointer user_data) {
    // For creates/deletes/renames, rescan
    if (event_type == G_FILE_MONITOR_EVENT_CREATED ||
        event_type == G_FILE_MONITOR_EVENT_DELETED ||
        event_type == G_FILE_MONITOR_EVENT_MOVED ||
        event_type == G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED ||
        event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
        AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
        browser->scan_directory(browser->current_directory);
        browser->populate_list();
        // Restart monitors to capture newly added/removed subfolders
        browser->start_dir_monitor(browser->current_directory);
    }
}

void AppImageBrowser::start_subdir_monitors(const std::string& path) {
    stop_subdir_monitors();
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                GFile* sub = g_file_new_for_path(entry.path().c_str());
                GError* error = nullptr;
                GFileMonitor* mon = g_file_monitor_directory(sub, G_FILE_MONITOR_NONE, nullptr, &error);
                g_object_unref(sub);
                if (mon) {
                    g_signal_connect(mon, "changed", G_CALLBACK(on_dir_changed), this);
                    sub_monitors.push_back(mon);
                } else if (error) {
                    g_error_free(error);
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        // ignore
    }
}

void AppImageBrowser::stop_subdir_monitors() {
    for (auto* m : sub_monitors) {
        g_object_unref(m);
    }
    sub_monitors.clear();
}

GtkWidget* AppImageBrowser::build_context_menu(const std::string& path) {
    GtkWidget* popover = gtk_popover_new();
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(popover), vbox);
    
    // Open button
    GtkWidget* open_btn = gtk_button_new_with_label("Open");
    g_object_set_data_full(G_OBJECT(open_btn), "appimage_path", g_strdup(path.c_str()), g_free);
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_menu_open_button_clicked), this);
    gtk_box_append(GTK_BOX(vbox), open_btn);
    
    // Show in folder button
    GtkWidget* show_btn = gtk_button_new_with_label("Show in Folder");
    g_object_set_data_full(G_OBJECT(show_btn), "appimage_path", g_strdup(path.c_str()), g_free);
    g_signal_connect(show_btn, "clicked", G_CALLBACK(on_menu_show_in_folder_button_clicked), this);
    gtk_box_append(GTK_BOX(vbox), show_btn);
    
    // Properties button
    GtkWidget* props_btn = gtk_button_new_with_label("Properties");
    g_object_set_data_full(G_OBJECT(props_btn), "appimage_path", g_strdup(path.c_str()), g_free);
    g_signal_connect(props_btn, "clicked", G_CALLBACK(on_menu_properties_button_clicked), this);
    gtk_box_append(GTK_BOX(vbox), props_btn);
    
    return popover;
}

void AppImageBrowser::on_item_clicked(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data) {
    // Check which button was clicked
    guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    
    if (button == GDK_BUTTON_SECONDARY) { // right click
        // Get the widget that received the click
        GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
        const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "appimage_path"));
        if (path) {
            AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
            GtkWidget* menu = browser->build_context_menu(path);
            gtk_popover_popup(GTK_POPOVER(menu));
        }
    }
    else if (button == GDK_BUTTON_PRIMARY && n_press == 2) { // double click
        // Launch the appimage
        GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
        const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "appimage_path"));
        if (path) {
            AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
            browser->launch_appimage(path);
        }
    }
}

void AppImageBrowser::on_menu_open_button_clicked(GtkButton* button, gpointer user_data) {
    const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(button), "appimage_path"));
    if (!path) return;
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    browser->launch_appimage(path);
}

void AppImageBrowser::on_menu_show_in_folder_button_clicked(GtkButton* button, gpointer user_data) {
    const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(button), "appimage_path"));
    if (!path) return;
    fs::path p(path);
    std::string dir = p.parent_path().string();
    // xdg-open directory
    GError* error = nullptr;
    GSubprocess* proc = g_subprocess_new(G_SUBPROCESS_FLAGS_NONE, &error, "xdg-open", dir.c_str(), nullptr);
    if (!proc) {
        if (error) g_error_free(error);
        return;
    }
    g_object_unref(proc);
}

void AppImageBrowser::on_menu_properties_button_clicked(GtkButton* button, gpointer user_data) {
    const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(button), "appimage_path"));
    if (!path) return;
    struct stat st;
    std::string info;
    if (stat(path, &st) == 0) {
        info += std::string("Path: ") + path + "\n";
        info += "Size: " + std::to_string((long long)st.st_size) + " bytes\n";
        info += std::string("Modified: ") + ctime(&st.st_mtime);
    } else {
        info = "Unable to stat file.";
    }
    AppImageBrowser* browser = static_cast<AppImageBrowser*>(user_data);
    
    // GTK4: Use AdwAlertDialog for modern alert (replaces deprecated AdwMessageDialog)
    AdwAlertDialog* dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Properties", info.c_str()));
    adw_alert_dialog_add_response(dialog, "ok", "OK");
    adw_alert_dialog_set_default_response(dialog, "ok");
    adw_alert_dialog_set_close_response(dialog, "ok");
    
    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(browser->window));
}

GtkWidget* AppImageBrowser::build_appimage_tile(const std::string& file) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_size_request(button, 200, 200);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_button_set_child(GTK_BUTTON(button), vbox);

    GtkWidget* top_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(top_spacer, TRUE);
    gtk_box_append(GTK_BOX(vbox), top_spacer);

    // Placeholder icon, real icon will be loaded asynchronously
    GtkWidget* icon = gtk_image_new_from_icon_name("application-x-executable");
    gtk_image_set_icon_size(GTK_IMAGE(icon), GTK_ICON_SIZE_LARGE);
    gtk_box_append(GTK_BOX(vbox), icon);

    fs::path p(file);
    std::string filename = p.filename().string();
    GtkWidget* label = gtk_label_new(filename.c_str());
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox), label);

    GtkWidget* bottom_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(bottom_spacer, TRUE);
    gtk_box_append(GTK_BOX(vbox), bottom_spacer);

    g_object_set_data_full(G_OBJECT(button), "appimage_path", g_strdup(file.c_str()), g_free);
    g_object_set_data(G_OBJECT(button), "img_widget", icon);
    g_object_set_data(G_OBJECT(button), "label_widget", label);
    g_signal_connect(button, "clicked", G_CALLBACK(on_appimage_clicked), this);
    
    // Add gesture click controller for right-click context menu
    GtkGestureClick* gesture = GTK_GESTURE_CLICK(gtk_gesture_click_new());
    gtk_widget_add_controller(button, GTK_EVENT_CONTROLLER(gesture));
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_item_clicked), this);

    return button;
}

std::string AppImageBrowser::extract_appimage_icon(const std::string& path) {
    // Create a temp dir
    gchar* tmpdir = g_dir_make_tmp("appimgXXXXXX", nullptr);
    if (!tmpdir) return "";
    std::string tmp(tmpdir);
    g_free(tmpdir);

    // Set APPIMAGE_EXTRACT_DIR and run "path --appimage-extract .DirIcon"
    GError* error = nullptr;
    auto flags = static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE);
    GSubprocessLauncher* launcher = g_subprocess_launcher_new(flags);
    g_subprocess_launcher_setenv(launcher, "APPIMAGE_EXTRACT_DIR", tmp.c_str(), TRUE);
    const char* argv1[] = { path.c_str(), "--appimage-extract", ".DirIcon", nullptr };
    GSubprocess* proc = g_subprocess_launcher_spawnv(launcher, argv1, &error);
    g_object_unref(launcher);
    if (!proc) {
        if (error) g_error_free(error);
        // cleanup tmp
        rmdir(tmp.c_str());
        return "";
    }
    // Wait briefly
    g_subprocess_wait(proc, nullptr, nullptr);
    g_object_unref(proc);

    std::string icon_candidate = tmp + "/squashfs-root/.DirIcon";
    if (g_file_test(icon_candidate.c_str(), G_FILE_TEST_IS_REGULAR)) {
        return icon_candidate;
    }
    // Cleanup if nothing found
    // Try removing created folder to avoid littering temp
    // We leave the directory if extraction created many files (best-effort cleanup skipped).
    return "";
}

struct LoadTask {
    AppImageBrowser* self;
    std::string path;
    GtkWidget* button;
};

gpointer AppImageBrowser::metadata_thread_func(gpointer data) {
    std::unique_ptr<LoadTask> task(static_cast<LoadTask*>(data));
    AppImageBrowser* self = task->self;
    const std::string path = task->path;
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return nullptr;
    std::time_t mtime = st.st_mtime;

    AppImageBrowser::CachedInfo info;
    info.mtime = mtime;

    // Use/prepare cache directory
    std::string cache_dir = self->get_cache_dir_for(path, mtime);

    // Try icon: prefer existing cached .DirIcon
    std::string icon_candidate = cache_dir + "/squashfs-root/.DirIcon";
    if (!g_file_test(icon_candidate.c_str(), G_FILE_TEST_IS_REGULAR)) {
        // Extract .DirIcon only (fast)
        GError* error = nullptr;
        auto flags = static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE);
        GSubprocessLauncher* launcher = g_subprocess_launcher_new(flags);
        g_subprocess_launcher_setenv(launcher, "APPIMAGE_EXTRACT_DIR", cache_dir.c_str(), TRUE);
        const char* argv1[] = { path.c_str(), "--appimage-extract", ".DirIcon", nullptr };
        GSubprocess* proc = g_subprocess_launcher_spawnv(launcher, argv1, &error);
        if (proc) {
            g_subprocess_wait(proc, nullptr, nullptr);
            g_object_unref(proc);
        }
        if (launcher) g_object_unref(launcher);
        if (error) g_error_free(error);
    }
    if (g_file_test(icon_candidate.c_str(), G_FILE_TEST_IS_REGULAR)) {
        info.icon_path = icon_candidate;
    }

    // Metadata: try to find a .desktop; if not found, consider full extract once
    std::string desktop_path;
    if (!self->find_desktop_file(cache_dir + "/squashfs-root", desktop_path)) {
        std::string marker = cache_dir + "/.extracted_all";
        if (!g_file_test(marker.c_str(), G_FILE_TEST_EXISTS)) {
            GError* error = nullptr;
            auto flags = static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE);
            GSubprocessLauncher* launcher = g_subprocess_launcher_new(flags);
            g_subprocess_launcher_setenv(launcher, "APPIMAGE_EXTRACT_DIR", cache_dir.c_str(), TRUE);
            const char* argv2[] = { path.c_str(), "--appimage-extract", nullptr };
            GSubprocess* proc = g_subprocess_launcher_spawnv(launcher, argv2, &error);
            if (proc) {
                g_subprocess_wait(proc, nullptr, nullptr);
                g_object_unref(proc);
            }
            if (launcher) g_object_unref(launcher);
            if (error) g_error_free(error);

            // write marker
            FILE* f = fopen(marker.c_str(), "w");
            if (f) { fputs("1", f); fclose(f); }
        }
        // try again
        self->find_desktop_file(cache_dir + "/squashfs-root", desktop_path);
    }

    if (!desktop_path.empty()) {
        self->parse_desktop_file(desktop_path, info);
        // If .desktop Icon key provided, try to resolve to a file within cache
        if (!info.icon_path.empty() && !g_file_test(info.icon_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
            // interpret icon_path as icon name; look for png/svg under cache
            std::string base = info.icon_path; // temporarily reuse field
            const char* exts[] = { ".png", ".svg" };
            const char* roots[] = { "/squashfs-root/usr/share/icons", "/squashfs-root/usr/share/pixmaps", "/squashfs-root" };
            bool found = false;
            for (auto root : roots) {
                for (auto ext : exts) {
                    std::string try_path = cache_dir + root + std::string("/") + base + ext;
                    if (g_file_test(try_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
                        info.icon_path = try_path;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) {
                // fallback: leave previous icon
                info.icon_path.clear();
            }
        }
    }

    // Save to in-memory cache
    {
        std::lock_guard<std::mutex> lock(self->cache_mutex_);
        self->cache_[path] = info;
    }

    // Push UI update onto main thread
    struct UiUpdate { AppImageBrowser* self; GtkWidget* button; std::string path; };
    UiUpdate* u = new UiUpdate{ self, task->button, path };
    g_idle_add_full(G_PRIORITY_DEFAULT, [](gpointer ud)->gboolean {
        std::unique_ptr<UiUpdate> uu(static_cast<UiUpdate*>(ud));
        AppImageBrowser::CachedInfo info;
        {
            std::lock_guard<std::mutex> lock(uu->self->cache_mutex_);
            auto it = uu->self->cache_.find(uu->path);
            if (it == uu->self->cache_.end()) return FALSE;
            info = it->second;
        }
        GtkWidget* img = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(uu->button), "img_widget"));
        GtkWidget* lbl = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(uu->button), "label_widget"));
        if (img && !info.icon_path.empty()) {
            gtk_image_set_from_file(GTK_IMAGE(img), info.icon_path.c_str());
        }
        if (lbl) {
            if (!info.name.empty()) {
                gtk_label_set_text(GTK_LABEL(lbl), info.name.c_str());
            }
        }
        return FALSE;
    }, u, nullptr);

    return nullptr;
}

void AppImageBrowser::load_metadata_async(const std::string& path, GtkWidget* button) {
    // Check if we already have fresh cache
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) {
        std::unique_lock<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end() && it->second.mtime == st.st_mtime) {
            // Cache is fresh; update UI immediately
            AppImageBrowser::CachedInfo info = it->second;
            lock.unlock();
            struct UiUpdate { AppImageBrowser* self; GtkWidget* button; AppImageBrowser::CachedInfo info; };
            UiUpdate* u = new UiUpdate{ this, button, info };
            g_idle_add_full(G_PRIORITY_DEFAULT, [](gpointer ud)->gboolean {
                std::unique_ptr<UiUpdate> uu(static_cast<UiUpdate*>(ud));
                const auto& info2 = uu->info;
                GtkWidget* img = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(uu->button), "img_widget"));
                GtkWidget* lbl = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(uu->button), "label_widget"));
                if (img && !info2.icon_path.empty()) {
                    gtk_image_set_from_file(GTK_IMAGE(img), info2.icon_path.c_str());
                }
                if (lbl && !info2.name.empty()) {
                    gtk_label_set_text(GTK_LABEL(lbl), info2.name.c_str());
                }
                return FALSE;
            }, u, nullptr);
            return;
        }
    }
    // Spawn background thread
    LoadTask* t = new LoadTask{ this, path, button };
    g_thread_new("appimage-meta", metadata_thread_func, t);
}

std::string AppImageBrowser::get_cache_dir_for(const std::string& path, std::time_t mtime) {
    gchar* cache_root = g_build_filename(g_get_user_cache_dir(), "gtkappfolder", nullptr);
    g_mkdir_with_parents(cache_root, 0700);
    std::string key = path + "#" + std::to_string((long long)mtime);
    gchar* sha = g_compute_checksum_for_string(G_CHECKSUM_SHA256, key.c_str(), -1);
    gchar* dir = g_build_filename(cache_root, sha, nullptr);
    g_mkdir_with_parents(dir, 0700);
    std::string result = dir;
    g_free(cache_root);
    g_free(sha);
    g_free(dir);
    return result;
}

bool AppImageBrowser::find_desktop_file(const std::string& root, std::string& out_path) {
    GDir* d = g_dir_open(root.c_str(), 0, nullptr);
    if (!d) return false;
    const gchar* name;
    while ((name = g_dir_read_name(d)) != nullptr) {
        std::string full = root + "/" + name;
        if (g_file_test(full.c_str(), G_FILE_TEST_IS_DIR)) {
            if (find_desktop_file(full, out_path)) { g_dir_close(d); return true; }
        } else {
            if (g_str_has_suffix(name, ".desktop")) {
                out_path = full;
                g_dir_close(d);
                return true;
            }
        }
    }
    g_dir_close(d);
    return false;
}

void AppImageBrowser::parse_desktop_file(const std::string& desktop_path, CachedInfo& info) {
    GKeyFile* kf = g_key_file_new();
    GError* error = nullptr;
    if (!g_key_file_load_from_file(kf, desktop_path.c_str(), G_KEY_FILE_NONE, &error)) {
        if (error) g_error_free(error);
        g_key_file_unref(kf);
        return;
    }
    // Standard desktop entry group
    gchar* name = g_key_file_get_string(kf, "Desktop Entry", "Name", nullptr);
    gchar* comment = g_key_file_get_string(kf, "Desktop Entry", "Comment", nullptr);
    gchar* icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", nullptr);
    if (name) { info.name = name; g_free(name); }
    if (comment) { info.comment = comment; g_free(comment); }
    if (icon) { info.icon_path = icon; g_free(icon); }
    g_key_file_unref(kf);
}

gboolean AppImageBrowser::on_drop(GtkDropTarget* target, const GValue* value, double x, double y, gpointer user_data) {
    AppImageBrowser* self = static_cast<AppImageBrowser*>(user_data);
    bool any = false;
    
    // GTK4 uses GValue to pass data; check if it's a string list or file list
    if (!value) return FALSE;
    
    GType type = G_VALUE_TYPE(value);
    
    // Handle GdkFileList (dragged files)
    if (gdk_file_list_get_type && type == gdk_file_list_get_type()) {
        GSList* files = (GSList*)g_value_get_boxed(value);
        for (GSList* l = files; l; l = l->next) {
            GFile* gf = (GFile*)l->data;
            gchar* path = g_file_get_path(gf);
            if (!path) continue;
            
            std::string src(path);
            g_free(path);
            
            // Only handle .AppImage files
            if (!g_str_has_suffix(src.c_str(), ".AppImage") && !g_str_has_suffix(src.c_str(), ".appimage"))
                continue;
            
            // Destination path
            fs::path p(src);
            std::string dest = self->current_directory + "/" + p.filename().string();
            
            // If exists, add numeric suffix
            int n = 1;
            while (g_file_test(dest.c_str(), G_FILE_TEST_EXISTS)) {
                fs::path base = p.filename();
                std::string stem = base.stem().string();
                std::string ext = base.extension().string();
                dest = self->current_directory + "/" + stem + " (" + std::to_string(n++) + ")" + ext;
            }
            
            GFile* gf_src = g_file_new_for_path(src.c_str());
            GFile* gf_dst = g_file_new_for_path(dest.c_str());
            GError* error = nullptr;
            if (g_file_copy(gf_src, gf_dst, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, &error)) {
                any = true;
            } else {
                if (error) g_error_free(error);
            }
            g_object_unref(gf_src);
            g_object_unref(gf_dst);
        }
    }
    // Handle URI list (dropped as text)
    else if (type == G_TYPE_STRING) {
        const gchar* uri_list = g_value_get_string(value);
        if (uri_list) {
            gchar** uris = g_uri_list_extract_uris(uri_list);
            if (uris) {
                for (int i = 0; uris[i] != nullptr; ++i) {
                    gchar* filename = g_filename_from_uri(uris[i], nullptr, nullptr);
                    if (!filename) continue;
                    
                    std::string src(filename);
                    g_free(filename);
                    
                    // Only handle .AppImage files
                    if (!g_str_has_suffix(src.c_str(), ".AppImage") && !g_str_has_suffix(src.c_str(), ".appimage"))
                        continue;
                    
                    // Destination path
                    fs::path p(src);
                    std::string dest = self->current_directory + "/" + p.filename().string();
                    
                    // If exists, add numeric suffix
                    int n = 1;
                    while (g_file_test(dest.c_str(), G_FILE_TEST_EXISTS)) {
                        fs::path base = p.filename();
                        std::string stem = base.stem().string();
                        std::string ext = base.extension().string();
                        dest = self->current_directory + "/" + stem + " (" + std::to_string(n++) + ")" + ext;
                    }
                    
                    GFile* gf_src = g_file_new_for_path(src.c_str());
                    GFile* gf_dst = g_file_new_for_path(dest.c_str());
                    GError* error = nullptr;
                    if (g_file_copy(gf_src, gf_dst, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, &error)) {
                        any = true;
                    } else {
                        if (error) g_error_free(error);
                    }
                    g_object_unref(gf_src);
                    g_object_unref(gf_dst);
                }
                g_strfreev(uris);
            }
        }
    }
    
    if (any) {
        self->set_status("File(s) added by drag-and-drop.");
        self->scan_directory(self->current_directory);
        self->populate_list();
        self->start_dir_monitor(self->current_directory);
    }
    
    return any;  // Return TRUE if we handled the drop
}

gboolean AppImageBrowser::on_autoclose_switch(GtkSwitch* /*widget*/, gboolean state, gpointer user_data) {
    AppImageBrowser* self = static_cast<AppImageBrowser*>(user_data);
    self->autoclose_enabled = state;
    if (self->settings) g_settings_set_boolean(self->settings, "autoclose-enabled", state);
    return FALSE; // allow state change to proceed
}

std::string AppImageBrowser::get_exe_dir() {
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0) return ".";
    buf[len] = '\0';
    std::string full(buf);
    fs::path p(full);
    return p.parent_path().string();
}

void AppImageBrowser::init_settings() {
    const char* schema_id = "com.github.gtkappfolder";
    // Try default schema source
    GSettingsSchemaSource* def = g_settings_schema_source_get_default();
    GSettingsSchema* found = def ? g_settings_schema_source_lookup(def, schema_id, TRUE) : nullptr;
    if (found) {
        settings = g_settings_new(schema_id);
        g_settings_schema_unref(found);
    } else {
        // Try local compiled schemas next to the binary (../schemas)
        std::string exe_dir = get_exe_dir();
        std::string local_dir = fs::path(exe_dir).parent_path().string() + "/schemas";
        GError* error = nullptr;
        GSettingsSchemaSource* src = g_settings_schema_source_new_from_directory(local_dir.c_str(), def, FALSE, &error);
        if (src) {
            GSettingsSchema* sch = g_settings_schema_source_lookup(src, schema_id, TRUE);
            if (sch) {
                settings = g_settings_new_full(sch, nullptr, nullptr);
                g_settings_schema_unref(sch);
            }
            g_settings_schema_source_unref(src);
        }
        if (error) g_error_free(error);
    }
    // Load values or defaults
    const char* home = g_get_home_dir();
    std::string home_dir = home ? std::string(home) : get_current_directory();
    if (settings) {
        autoclose_enabled = g_settings_get_boolean(settings, "autoclose-enabled");
        gchar* last = g_settings_get_string(settings, "last-folder");
        if (last && *last) {
            if (g_file_test(last, G_FILE_TEST_IS_DIR)) {
                current_directory = last;
            }
        }
        if (last) g_free(last);
    }
    if (current_directory.empty()) current_directory = home_dir;
}
