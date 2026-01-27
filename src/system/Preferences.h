#pragma once

#include <gio/gio.h>
#include <string>

namespace System {

/**
 * Wraps GSettings for application preferences management.
 * Provides type-safe access to application settings.
 */
class Preferences {
public:
    /**
     * Initializes preferences with the given schema ID.
     * 
     * @param schema_id GSettings schema identifier (e.g., "com.github.gtkappfolder")
     */
    explicit Preferences(const std::string& schema_id);
    ~Preferences();
    
    /**
     * Gets a boolean preference value.
     * 
     * @param key Settings key name
     * @param default_value Default value if key doesn't exist
     * @return Boolean value
     */
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    /**
     * Sets a boolean preference value.
     * 
     * @param key Settings key name
     * @param value Value to set
     */
    void set_bool(const std::string& key, bool value);
    
    /**
     * Gets a string preference value.
     * 
     * @param key Settings key name
     * @param default_value Default value if key doesn't exist
     * @return String value
     */
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * Sets a string preference value.
     * 
     * @param key Settings key name
     * @param value Value to set
     */
    void set_string(const std::string& key, const std::string& value);
    
    /**
     * Gets an integer preference value.
     * 
     * @param key Settings key name
     * @param default_value Default value if key doesn't exist
     * @return Integer value
     */
    int get_int(const std::string& key, int default_value = 0) const;
    
    /**
     * Sets an integer preference value.
     * 
     * @param key Settings key name
     * @param value Value to set
     */
    void set_int(const std::string& key, int value);
    
    /**
     * Checks if settings are available.
     * 
     * @return true if GSettings is initialized, false otherwise
     */
    bool is_available() const;
    
private:
    GSettings* settings;
};

} // namespace System
