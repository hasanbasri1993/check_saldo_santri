#include "config_manager.h"

// =============================================
// CONFIG MANAGER IMPLEMENTATION
// =============================================

const char* ConfigManager::NAMESPACE = "device_config";

ConfigManager::ConfigManager() : initialized(false) {
    // Constructor - config will be loaded in begin()
}

bool ConfigManager::begin() {
    Serial.println("=== CONFIG MANAGER INITIALIZATION (Preferences) ===");
    
    // Initialize Preferences
    initializePreferences();
    
    // Test Preferences persistence
    testPreferencesPersistence();
    
    // Load configuration
    Serial.println("Attempting to load configuration from Preferences...");
    if (!loadConfig()) {
        Serial.println("*** FAILED TO LOAD CONFIG - USING DEFAULTS ***");
        Serial.println("This means either:");
        Serial.println("1. Preferences is uninitialized (first boot)");
        Serial.println("2. Preferences data is corrupted");
        Serial.println("3. Configuration validation failed");
        resetToDefaults();
        Serial.println("Saving defaults to Preferences...");
        saveConfig();
    } else {
        Serial.println("*** CONFIGURATION LOADED SUCCESSFULLY ***");
    }
    
    initialized = true;
    Serial.println("=== CONFIG MANAGER INITIALIZATION COMPLETE ===");
    Serial.printf("Final Configuration:\n");
    Serial.printf("  API Base URL: %s\n", config.apiBaseUrl);
    Serial.printf("  mDNS Hostname: %s\n", config.mdnsHostname);
    Serial.printf("  Device Name: %s\n", config.deviceName);
    Serial.printf("  Config Valid: %d\n", config.configValid);
    
    return true;
}

void ConfigManager::end() {
    preferences.end();
    initialized = false;
    Serial.println("Config Manager ended");
}

void ConfigManager::initializePreferences() {
    Serial.println("Initializing Preferences...");
    bool success = preferences.begin(NAMESPACE, false);
    if (success) {
        Serial.printf("Preferences initialized with namespace: %s\n", NAMESPACE);
    } else {
        Serial.println("Failed to initialize Preferences!");
    }
}

bool ConfigManager::loadConfig() {
    Serial.println("Loading configuration from Preferences...");
    
    // Check if preferences has any keys by trying to read a key
    String testApiUrl = preferences.getString("apiBaseUrl", "");
    if (testApiUrl.length() == 0) {
        Serial.println("No configuration found in Preferences");
        return false;
    }
    
    // Load individual fields
    String apiUrl = preferences.getString("apiBaseUrl", "");
    String hostname = preferences.getString("mdnsHostname", "");
    String deviceName = preferences.getString("deviceName", "");
    uint8_t configValid = preferences.getUChar("configValid", 0);
    
    Serial.printf("Loaded from Preferences - Valid: %d, API: %s, Hostname: %s, Device: %s\n", 
                  configValid, apiUrl.c_str(), hostname.c_str(), deviceName.c_str());
    
    // Validate loaded data
    if (configValid == 1 && apiUrl.length() > 0 && hostname.length() > 0 && deviceName.length() > 0) {
        // Copy to config structure
        strncpy(config.apiBaseUrl, apiUrl.c_str(), sizeof(config.apiBaseUrl) - 1);
        config.apiBaseUrl[sizeof(config.apiBaseUrl) - 1] = '\0';
        
        strncpy(config.mdnsHostname, hostname.c_str(), sizeof(config.mdnsHostname) - 1);
        config.mdnsHostname[sizeof(config.mdnsHostname) - 1] = '\0';
        
        strncpy(config.deviceName, deviceName.c_str(), sizeof(config.deviceName) - 1);
        config.deviceName[sizeof(config.deviceName) - 1] = '\0';
        
        config.configValid = configValid;
        
        // Validate the loaded config
        if (validateConfig(config)) {
            Serial.println("Configuration loaded and validated successfully");
            return true;
        } else {
            Serial.println("Configuration validation failed");
        }
    } else {
        Serial.println("Invalid configuration data in Preferences");
    }
    
    return false;
}

bool ConfigManager::saveConfig() {
    Serial.println("=== SAVING CONFIGURATION TO PREFERENCES ===");
    Serial.printf("Saving config - Valid: %d, API: %s, Hostname: %s, Device: %s\n", 
                  config.configValid,
                  config.apiBaseUrl,
                  config.mdnsHostname,
                  config.deviceName);
    
    if (!validateConfig()) {
        Serial.println("Configuration validation failed - cannot save");
        return false;
    }
    
    // Save individual fields with debug
    bool success = true;
    
    Serial.println("Saving individual fields...");
    bool apiResult = preferences.putString("apiBaseUrl", config.apiBaseUrl);
    Serial.printf("  apiBaseUrl save: %s\n", apiResult ? "SUCCESS" : "FAILED");
    success &= apiResult;
    
    bool hostnameResult = preferences.putString("mdnsHostname", config.mdnsHostname);
    Serial.printf("  mdnsHostname save: %s\n", hostnameResult ? "SUCCESS" : "FAILED");
    success &= hostnameResult;
    
    bool deviceResult = preferences.putString("deviceName", config.deviceName);
    Serial.printf("  deviceName save: %s\n", deviceResult ? "SUCCESS" : "FAILED");
    success &= deviceResult;
    
    bool validResult = preferences.putUChar("configValid", config.configValid);
    Serial.printf("  configValid save: %s\n", validResult ? "SUCCESS" : "FAILED");
    success &= validResult;
    
    Serial.printf("Overall save result: %s\n", success ? "SUCCESS" : "FAILED");
    
    if (success) {
        Serial.println("*** CONFIGURATION SAVED SUCCESSFULLY ***");
        
        // Verify the save by reading it back
        String verifyApiUrl = preferences.getString("apiBaseUrl", "");
        String verifyHostname = preferences.getString("mdnsHostname", "");
        String verifyDeviceName = preferences.getString("deviceName", "");
        uint8_t verifyConfigValid = preferences.getUChar("configValid", 0);
        
        Serial.printf("Verification - Valid: %d, API: %s, Hostname: %s, Device: %s\n", 
                      verifyConfigValid,
                      verifyApiUrl.c_str(),
                      verifyHostname.c_str(),
                      verifyDeviceName.c_str());
        
        // Check if verification matches what we saved
        bool verificationPassed = (verifyApiUrl == config.apiBaseUrl) &&
                                  (verifyHostname == config.mdnsHostname) &&
                                  (verifyDeviceName == config.deviceName) &&
                                  (verifyConfigValid == config.configValid);
        
        if (verificationPassed) {
            Serial.println("*** VERIFICATION PASSED - DATA PERSISTED CORRECTLY ***");
        } else {
            Serial.println("*** VERIFICATION FAILED - DATA MISMATCH ***");
            success = false;
        }
    } else {
        Serial.println("*** FAILED TO SAVE CONFIGURATION ***");
    }
    
    return success;
}

void ConfigManager::resetToDefaults() {
    Serial.println("Resetting configuration to defaults");
    config = DeviceConfig(); // Use default constructor
    
    Serial.printf("Default config - Valid: %d, API: %s, Hostname: %s, Device: %s\n", 
                  config.configValid,
                  config.apiBaseUrl,
                  config.mdnsHostname,
                  config.deviceName);
}

void ConfigManager::clearPreferences() {
    Serial.println("Clearing Preferences...");
    preferences.clear();
    Serial.println("Preferences cleared");
}

const char* ConfigManager::getApiBaseUrl() {
    return config.apiBaseUrl;
}

const char* ConfigManager::getMdnsHostname() {
    return config.mdnsHostname;
}

const char* ConfigManager::getDeviceName() {
    return config.deviceName;
}

DeviceConfig& ConfigManager::getConfig() {
    return config;
}

bool ConfigManager::setApiBaseUrl(const char* url) {
    if (url == nullptr) {
        Serial.println("Invalid API URL: null pointer");
        return false;
    }
    if (!isValidUrl(url)) {
        Serial.printf("Invalid API URL: %s\n", url);
        return false;
    }
    
    strncpy(config.apiBaseUrl, url, sizeof(config.apiBaseUrl) - 1);
    config.apiBaseUrl[sizeof(config.apiBaseUrl) - 1] = '\0';
    
    Serial.printf("API Base URL updated to: %s\n", config.apiBaseUrl);
    return true;
}

bool ConfigManager::setMdnsHostname(const char* hostname) {
    if (hostname == nullptr) {
        Serial.println("Invalid mDNS hostname: null pointer");
        return false;
    }
    if (!isValidHostname(hostname)) {
        Serial.printf("Invalid mDNS hostname: %s\n", hostname);
        return false;
    }
    
    strncpy(config.mdnsHostname, hostname, sizeof(config.mdnsHostname) - 1);
    config.mdnsHostname[sizeof(config.mdnsHostname) - 1] = '\0';
    
    Serial.printf("mDNS hostname updated to: %s\n", config.mdnsHostname);
    return true;
}

bool ConfigManager::setDeviceName(const char* name) {
    if (name == nullptr) {
        Serial.println("Invalid device name: null pointer");
        return false;
    }
    
    size_t len = strnlen(name, 64);
    if (len == 0 || len >= 64) {
        Serial.printf("Invalid device name length: %s\n", name);
        return false;
    }
    
    strncpy(config.deviceName, name, sizeof(config.deviceName) - 1);
    config.deviceName[sizeof(config.deviceName) - 1] = '\0';
    
    Serial.printf("Device name updated to: %s\n", config.deviceName);
    return true;
}

bool ConfigManager::isValidUrl(const char* url) {
    if (!url) return false;
    size_t len = strnlen(url, 256);
    if (len == 0 || len >= 256) return false;
    
    // Basic URL validation
    String urlStr = String(url);
    return urlStr.startsWith("http://") || urlStr.startsWith("https://");
}

bool ConfigManager::isValidHostname(const char* hostname) {
    if (!hostname) return false;
    size_t len = strnlen(hostname, 64);
    if (len == 0 || len > 32) return false;
    
    // Basic hostname validation (alphanumeric and hyphens only)
    for (size_t i = 0; i < len; i++) {
        char c = hostname[i];
        if (!isalnum(c) && c != '-') {
            return false;
        }
    }
    return true;
}

bool ConfigManager::validateConfig() {
    return isValidUrl(config.apiBaseUrl) && isValidHostname(config.mdnsHostname) && strlen(config.deviceName) > 0;
}

bool ConfigManager::validateConfig(const DeviceConfig& testConfig) {
    return isValidUrl(testConfig.apiBaseUrl) && isValidHostname(testConfig.mdnsHostname) && strlen(testConfig.deviceName) > 0;
}

String ConfigManager::toJson() {
    JsonDocument doc;
    doc["apiBaseUrl"] = config.apiBaseUrl;
    doc["mdnsHostname"] = config.mdnsHostname;
    doc["deviceName"] = config.deviceName;
    doc["configValid"] = config.configValid;
    
    String json;
    serializeJson(doc, json);
    return json;
}

bool ConfigManager::fromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return false;
    }
    
    // Extract values
    const char* apiUrl = doc["apiBaseUrl"];
    const char* hostname = doc["mdnsHostname"];
    const char* deviceName = doc["deviceName"];
    
    if (!apiUrl || !hostname || !deviceName) {
        Serial.println("Missing required fields in JSON");
        return false;
    }
    
    // Validate and set
    if (!setApiBaseUrl(apiUrl) || !setMdnsHostname(hostname) || !setDeviceName(deviceName)) {
        return false;
    }
    
    return true;
}

void ConfigManager::printPreferencesContent() {
    Serial.println("=== PREFERENCES CONTENT ===");
    Serial.printf("Namespace: %s\n", NAMESPACE);
    
    // List all keys (this is a simplified version)
    Serial.println("Available keys:");
    Serial.println("- apiBaseUrl");
    Serial.println("- mdnsHostname");
    Serial.println("- deviceName");
    Serial.println("- configValid");
}

void ConfigManager::testPreferencesPersistence() {
    Serial.println("=== TESTING PREFERENCES PERSISTENCE ===");
    
    // Write test data
    const char* testKey = "test_key";
    const char* testValue = "TEST123";
    
    Serial.printf("Writing test data '%s' with key '%s'\n", testValue, testKey);
    bool writeSuccess = preferences.putString(testKey, testValue);
    
    if (writeSuccess) {
        // Read it back immediately
        String readBack = preferences.getString(testKey, "");
        Serial.printf("Read back: '%s'\n", readBack.c_str());
        
        if (readBack == testValue) {
            Serial.println("Preferences write/read test PASSED");
        } else {
            Serial.println("Preferences write/read test FAILED");
        }
        
        // Clean up test data
        preferences.remove(testKey);
        Serial.println("Test data cleaned up");
    } else {
        Serial.println("Preferences write test FAILED");
    }
    
    Serial.println("=== PREFERENCES PERSISTENCE TEST COMPLETE ===");
}

// =============================================
// GLOBAL INSTANCE
// =============================================

ConfigManager configManager;