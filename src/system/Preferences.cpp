#include "Preferences.h"
#include <iostream>

namespace System {

Preferences::Preferences(const std::string& schema_id) : settings(nullptr) {
    // Check if schema is available
    GSettingsSchemaSource* source = g_settings_schema_source_get_default();
    if (source) {
        GSettingsSchema* schema = g_settings_schema_source_lookup(source, schema_id.c_str(), FALSE);
        if (schema) {
            settings = g_settings_new(schema_id.c_str());
            g_settings_schema_unref(schema);
        } else {
            std::cerr << "GSettings schema not found: " << schema_id << std::endl;
        }
    }
}

Preferences::~Preferences() {
    if (settings) {
        g_object_unref(settings);
    }
}

bool Preferences::is_available() const {
    return settings != nullptr;
}

bool Preferences::get_bool(const std::string& key, bool default_value) const {
    if (!settings) {
        return default_value;
    }
    return g_settings_get_boolean(settings, key.c_str());
}

void Preferences::set_bool(const std::string& key, bool value) {
    if (settings) {
        g_settings_set_boolean(settings, key.c_str(), value);
    }
}

std::string Preferences::get_string(const std::string& key, const std::string& default_value) const {
    if (!settings) {
        return default_value;
    }
    
    gchar* value = g_settings_get_string(settings, key.c_str());
    std::string result = value ? value : default_value;
    g_free(value);
    return result;
}

void Preferences::set_string(const std::string& key, const std::string& value) {
    if (settings) {
        g_settings_set_string(settings, key.c_str(), value.c_str());
    }
}

int Preferences::get_int(const std::string& key, int default_value) const {
    if (!settings) {
        return default_value;
    }
    return g_settings_get_int(settings, key.c_str());
}

void Preferences::set_int(const std::string& key, int value) {
    if (settings) {
        g_settings_set_int(settings, key.c_str(), value);
    }
}

} // namespace System
