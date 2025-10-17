#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// =============================================
// CONFIGURATION STRUCTURE
// =============================================

struct DeviceConfig {
    char apiBaseUrl[128];
    char mdnsHostname[32];
    char deviceName[64];
    uint8_t configValid;  // Use uint8_t instead of bool for better EEPROM compatibility

    // Constructor with defaults
    DeviceConfig() {
        strcpy(apiBaseUrl, "http://192.168.87.83:7894");
        strcpy(mdnsHostname, "santri-reader");
        strcpy(deviceName, "Santri Card Reader");
        configValid = 1;  // Use 1 instead of true
    }
};

// =============================================
// CONFIG MANAGER CLASS
// =============================================

class ConfigManager {
private:
    Preferences preferences;
    DeviceConfig config;
    bool initialized;
    static const char* NAMESPACE;

public:
    ConfigManager();
    
    // Initialization
    bool begin();
    void end();
    
    // Configuration management
    bool loadConfig();
    bool saveConfig();
    void resetToDefaults();
    void clearPreferences();
    
    // Getters
    const char* getApiBaseUrl();
    const char* getMdnsHostname();
    const char* getDeviceName();
    DeviceConfig& getConfig();
    
    // Setters
    bool setApiBaseUrl(const char* url);
    bool setMdnsHostname(const char* hostname);
    bool setDeviceName(const char* name);
    
    // Debug functions
    void printPreferencesContent();
    void testPreferencesPersistence();
    
    // Validation
    bool isValidUrl(const char* url);
    bool isValidHostname(const char* hostname);
    
    // JSON conversion
    String toJson();
    bool fromJson(const String& json);
    
private:
    void initializePreferences();
    bool validateConfig();
    bool validateConfig(const DeviceConfig& testConfig);
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
