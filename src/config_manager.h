#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// =============================================
// CONFIGURATION STRUCTURE
// =============================================

struct DeviceConfig {
    char apiBaseUrl[128];
    char mdnsHostname[32];
    bool configValid;
    
    // Constructor with defaults
    DeviceConfig() {
        strcpy(apiBaseUrl, "http://192.168.87.83:7894");
        strcpy(mdnsHostname, "santri-reader");
        configValid = true;
    }
};

// =============================================
// CONFIG MANAGER CLASS
// =============================================

class ConfigManager {
private:
    static const int EEPROM_SIZE = 512;
    static const int CONFIG_ADDRESS = 0;
    DeviceConfig config;
    bool initialized;

public:
    ConfigManager();
    
    // Initialization
    bool begin();
    void end();
    
    // Configuration management
    bool loadConfig();
    bool saveConfig();
    void resetToDefaults();
    void clearEEPROM();
    
    // Getters
    const char* getApiBaseUrl();
    const char* getMdnsHostname();
    DeviceConfig& getConfig();
    
    // Setters
    bool setApiBaseUrl(const char* url);
    bool setMdnsHostname(const char* hostname);
    
    // Validation
    bool isValidUrl(const char* url);
    bool isValidHostname(const char* hostname);
    
    // JSON conversion
    String toJson();
    bool fromJson(const String& json);
    
private:
    void initializeEEPROM();
    bool validateConfig();
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
