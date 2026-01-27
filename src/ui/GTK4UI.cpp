#include "GTK4UI.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace UI {

GTK4UI::GTK4UI() 
    : window(nullptr), main_box(nullptr), path_entry(nullptr),
      refresh_button(nullptr), scrolled_window(nullptr), flow_box(nullptr),
      status_label(nullptr), path_completion(nullptr), completion_model(nullptr) {
}

GTK4UI::~GTK4UI() {
    if (completion_model) {
        g_object_unref(completion_model);
    }
    if (path_completion) {
        g_object_unref(path_completion);
    }
}

void GTK4UI::initialize(int argc, char* argv[]) {
    adw_init();
}

void GTK4UI::create_window(const std::string& title, int width, int height) {
    window = ADW_APPLICATION_WINDOW(adw_application_window_new(nullptr));
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    g_signal_connect(window, "close-request", G_CALLBACK(on_close_request_cb), this);
    
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    adw_application_window_set_content(window, main_box);
}

void GTK4UI::show_and_run() {
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
    // Note: GTK4 typically uses GtkApplication main loop
}

void GTK4UI::quit() {
    gtk_window_close(GTK_WINDOW(window));
}

void GTK4UI::create_path_bar(const std::string& initial_path) {
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(main_box), hbox);
    
    GtkWidget* label = gtk_label_new("Directory:");
    gtk_box_append(GTK_BOX(hbox), label);
    
    path_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(path_entry), initial_path.c_str());
    gtk_widget_set_hexpand(path_entry, TRUE);
    gtk_box_append(GTK_BOX(hbox), path_entry);
    g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_activate_cb), this);
    g_signal_connect(path_entry, "changed", G_CALLBACK(on_path_entry_changed_cb), this);
    GtkGestureClick* click = GTK_GESTURE_CLICK(gtk_gesture_click_new());
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect(click, "pressed", G_CALLBACK(on_path_click_cb), this);
    gtk_widget_add_controller(path_entry, GTK_EVENT_CONTROLLER(click));
    
    refresh_button = gtk_button_new_with_label("Refresh");
    gtk_box_append(GTK_BOX(hbox), refresh_button);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked_cb), this);
}

void GTK4UI::create_content_area() {
    scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(main_box), scrolled_window);
    
    flow_box = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flow_box), TRUE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow_box), 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), flow_box);
}

void GTK4UI::create_status_bar(const std::string& initial_text) {
    status_label = gtk_label_new(initial_text.c_str());
    gtk_box_append(GTK_BOX(main_box), status_label);
}

void GTK4UI::setup_path_completion() {
    completion_model = gtk_list_store_new(1, G_TYPE_STRING);
    path_completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(path_completion, GTK_TREE_MODEL(completion_model));
    gtk_entry_completion_set_text_column(path_completion, 0);
    gtk_entry_completion_set_inline_completion(path_completion, FALSE);
    gtk_entry_completion_set_inline_selection(path_completion, FALSE);
    gtk_entry_completion_set_popup_completion(path_completion, TRUE);
    gtk_entry_completion_set_minimum_key_length(path_completion, 1);
    gtk_entry_completion_set_match_func(path_completion, on_completion_match_cb, this, nullptr);
    g_signal_connect(path_completion, "match-selected", G_CALLBACK(on_completion_match_selected_cb), this);
    gtk_entry_set_completion(GTK_ENTRY(path_entry), path_completion);
}

void GTK4UI::update_path_completion(const std::string& current_text) {
    if (!completion_model) return;
    
    gtk_list_store_clear(completion_model);
    
    std::string base_path = current_text;
    std::string search_term = "";
    
    size_t last_sep = current_text.find_last_of('/');
    if (last_sep != std::string::npos) {
        base_path = current_text.substr(0, last_sep);
        search_term = current_text.substr(last_sep + 1);
    }
    
    if (base_path.empty()) base_path = "/";

    // Add parent directory option
    {
        GtkTreeIter iter;
        gtk_list_store_append(completion_model, &iter);
        gtk_list_store_set(completion_model, &iter, 0, "..", -1);
    }
    
    try {
        if (!fs::exists(base_path) || !fs::is_directory(base_path)) return;
        
        std::vector<std::string> matches;
        for (const auto& entry : fs::directory_iterator(base_path)) {
            if (entry.is_directory()) {
                std::string dirname = entry.path().filename().string();
                if (search_term.empty() || dirname.find(search_term) != std::string::npos) {
                    matches.push_back(entry.path().string());
                }
            }
        }
        
        std::sort(matches.begin(), matches.end());
        
        int count = 0;
        for (const auto& match : matches) {
            if (count++ >= 20) break;
            GtkTreeIter iter;
            gtk_list_store_append(completion_model, &iter);
            gtk_list_store_set(completion_model, &iter, 0, match.c_str(), -1);
        }
    } catch (const fs::filesystem_error&) {
        // Ignore errors
    }
}

std::string GTK4UI::get_path_entry_text() const {
    return gtk_editable_get_text(GTK_EDITABLE(path_entry));
}

void GTK4UI::set_path_entry_text(const std::string& text) {
    gtk_editable_set_text(GTK_EDITABLE(path_entry), text.c_str());
}

void GTK4UI::clear_content() {
    GtkWidget* child = gtk_widget_get_first_child(flow_box);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_flow_box_remove(GTK_FLOW_BOX(flow_box), child);
        child = next;
    }
}

void GTK4UI::add_appimage_tile(const std::string& path, const std::string& filename,
                                const std::string& icon_path) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_size_request(button, 200, 200);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_button_set_child(GTK_BUTTON(button), vbox);
    
    GtkWidget* top_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(top_spacer, TRUE);
    gtk_box_append(GTK_BOX(vbox), top_spacer);
    
    GtkWidget* icon = gtk_image_new_from_icon_name("application-x-executable");
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 96);
    gtk_box_append(GTK_BOX(vbox), icon);
    
    GtkWidget* label = gtk_label_new(filename.c_str());
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox), label);
    
    GtkWidget* bottom_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(bottom_spacer, TRUE);
    gtk_box_append(GTK_BOX(vbox), bottom_spacer);
    
    g_object_set_data_full(G_OBJECT(button), "appimage_path", g_strdup(path.c_str()), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(on_appimage_clicked_cb), this);
    
    gtk_flow_box_append(GTK_FLOW_BOX(flow_box), button);
}

void GTK4UI::show_empty_message(const std::string& message) {
    GtkWidget* label = gtk_label_new(message.c_str());
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_flow_box_append(GTK_FLOW_BOX(flow_box), label);
}

void GTK4UI::refresh_content() {
    // GTK4 automatically shows widgets
}

void GTK4UI::set_path_changed_callback(PathChangedCallback callback) {
    on_path_changed = callback;
}

void GTK4UI::set_refresh_callback(RefreshCallback callback) {
    on_refresh = callback;
}

void GTK4UI::set_appimage_click_callback(AppImageClickCallback callback) {
    on_appimage_click = callback;
}

// Static callbacks
gboolean GTK4UI::on_close_request_cb(GtkWidget* widget, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    ui->quit();
    return FALSE;
}

void GTK4UI::on_path_activate_cb(GtkEntry* entry, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    if (ui->on_path_changed) {
        ui->on_path_changed(ui->get_path_entry_text());
    }
}

void GTK4UI::on_refresh_clicked_cb(GtkButton* button, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    if (ui->on_refresh) {
        ui->on_refresh();
    }
}

void GTK4UI::on_appimage_clicked_cb(GtkButton* button, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    const char* path = static_cast<const char*>(g_object_get_data(G_OBJECT(button), "appimage_path"));
    if (path && ui->on_appimage_click) {
        ui->on_appimage_click(path);
    }
}

void GTK4UI::on_path_entry_changed_cb(GtkEditable* editable, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    const char* text = gtk_editable_get_text(editable);
    if (text && strlen(text) > 0) {
        ui->update_path_completion(text);
    }
}

void GTK4UI::on_path_click_cb(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    if (ui->path_completion) {
        gtk_entry_completion_complete(ui->path_completion);
        gtk_editable_set_position(GTK_EDITABLE(ui->path_entry), -1);
    }
}

gboolean GTK4UI::on_completion_match_cb(GtkEntryCompletion* completion, 
                                        const gchar* key, GtkTreeIter* iter, 
                                        gpointer user_data) {
    GtkTreeModel* model = gtk_entry_completion_get_model(completion);
    gchar* item_text = nullptr;
    gtk_tree_model_get(model, iter, 0, &item_text, -1);
    
    if (!item_text) return FALSE;
    
    std::string item(item_text);
    std::string search_key(key ? key : "");
    g_free(item_text);

    if (item == "..") {
        return TRUE;
    }
    
    return item.find(search_key) != std::string::npos;
}

gboolean GTK4UI::on_completion_match_selected_cb(GtkEntryCompletion* completion,
                                                 GtkTreeModel* model,
                                                 GtkTreeIter* iter,
                                                 gpointer user_data) {
    GTK4UI* ui = static_cast<GTK4UI*>(user_data);
    gchar* item_text = nullptr;
    gtk_tree_model_get(model, iter, 0, &item_text, -1);
    if (!item_text) {
        return FALSE;
    }

    std::string path(item_text);
    g_free(item_text);

    if (path == "..") {
        std::string current = ui->get_path_entry_text();
        // Trim trailing slash
        if (!current.empty() && current.back() == '/') {
            current.pop_back();
        }
        size_t last_sep = current.find_last_of('/');
        if (last_sep == std::string::npos) {
            path = "/";
        } else if (last_sep == 0) {
            path = "/";
        } else {
            path = current.substr(0, last_sep + 1);
        }
    }

    if (!path.empty() && path.back() != '/') {
        path.push_back('/');
    }

    ui->set_path_entry_text(path);
    if (ui->on_path_changed) {
        ui->on_path_changed(path);
    }

    gtk_editable_select_region(GTK_EDITABLE(ui->path_entry), 0, -1);
    if (ui->path_completion) {
        gtk_entry_completion_complete(ui->path_completion);
    }

    return TRUE;
}

} // namespace UI
