#include "config_manager.h"

// =============================================
// CONFIG MANAGER IMPLEMENTATION
// =============================================

ConfigManager::ConfigManager() : initialized(false) {
    // Constructor - config will be loaded in begin()
}

bool ConfigManager::begin() {
    Serial.println("Initializing Config Manager...");
    
    // Initialize EEPROM
    initializeEEPROM();
    
    // Debug: Print EEPROM content
    Serial.println("EEPROM content (first 32 bytes):");
    for (int i = 0; i < 32; i++) {
        uint8_t byte = EEPROM.read(i);
        Serial.printf("%02X ", byte);
        if ((i + 1) % 16 == 0) Serial.println();
    }
    Serial.println();
    
    // Load configuration
    if (!loadConfig()) {
        Serial.println("Failed to load config, using defaults");
        resetToDefaults();
        saveConfig();
    }
    
    initialized = true;
    Serial.println("Config Manager initialized successfully");
    Serial.printf("API Base URL: %s\n", config.apiBaseUrl);
    Serial.printf("mDNS Hostname: %s\n", config.mdnsHostname);
    
    return true;
}

void ConfigManager::end() {
    EEPROM.end();
    initialized = false;
    Serial.println("Config Manager ended");
}

void ConfigManager::initializeEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    Serial.printf("EEPROM initialized with %d bytes\n", EEPROM_SIZE);
}

bool ConfigManager::loadConfig() {
    Serial.println("Loading configuration from EEPROM...");
    
    // Read config from EEPROM
    DeviceConfig tempConfig;
    EEPROM.get(CONFIG_ADDRESS, tempConfig);
    
    // Check for uninitialized EEPROM (all 0xFF bytes)
    bool isUninitialized = true;
    for (int i = 0; i < sizeof(DeviceConfig); i++) {
        if (((uint8_t*)&tempConfig)[i] != 0xFF) {
            isUninitialized = false;
            break;
        }
    }
    
    if (isUninitialized) {
        Serial.println("EEPROM is uninitialized (all 0xFF)");
        return false;
    }
    
    // Validate config structure
    if (tempConfig.configValid && validateConfig()) {
        config = tempConfig;
        Serial.println("Configuration loaded successfully");
        return true;
    }
    
    Serial.println("Invalid configuration in EEPROM - data may be corrupted");
    return false;
}

bool ConfigManager::saveConfig() {
    Serial.println("Saving configuration to EEPROM...");
    
    if (!validateConfig()) {
        Serial.println("Configuration validation failed");
        return false;
    }
    
    // Write config to EEPROM
    EEPROM.put(CONFIG_ADDRESS, config);
    bool success = EEPROM.commit();
    
    if (success) {
        Serial.println("Configuration saved successfully");
    } else {
        Serial.println("Failed to save configuration");
    }
    
    return success;
}

void ConfigManager::resetToDefaults() {
    Serial.println("Resetting configuration to defaults");
    config = DeviceConfig(); // Use default constructor
    
    // Clear EEPROM to ensure clean state
    clearEEPROM();
}

void ConfigManager::clearEEPROM() {
    Serial.println("Clearing EEPROM...");
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared");
}

const char* ConfigManager::getApiBaseUrl() {
    return config.apiBaseUrl;
}

const char* ConfigManager::getMdnsHostname() {
    return config.mdnsHostname;
}

DeviceConfig& ConfigManager::getConfig() {
    return config;
}

bool ConfigManager::setApiBaseUrl(const char* url) {
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
    if (!isValidHostname(hostname)) {
        Serial.printf("Invalid mDNS hostname: %s\n", hostname);
        return false;
    }
    
    strncpy(config.mdnsHostname, hostname, sizeof(config.mdnsHostname) - 1);
    config.mdnsHostname[sizeof(config.mdnsHostname) - 1] = '\0';
    
    Serial.printf("mDNS hostname updated to: %s\n", config.mdnsHostname);
    return true;
}

bool ConfigManager::isValidUrl(const char* url) {
    if (!url || strlen(url) == 0) return false;
    
    // Basic URL validation
    String urlStr = String(url);
    return urlStr.startsWith("http://") || urlStr.startsWith("https://");
}

bool ConfigManager::isValidHostname(const char* hostname) {
    if (!hostname || strlen(hostname) == 0) return false;
    
    // Basic hostname validation (alphanumeric and hyphens only)
    for (int i = 0; i < strlen(hostname); i++) {
        char c = hostname[i];
        if (!isalnum(c) && c != '-') {
            return false;
        }
    }
    
    return strlen(hostname) <= 32;
}

bool ConfigManager::validateConfig() {
    return isValidUrl(config.apiBaseUrl) && isValidHostname(config.mdnsHostname);
}

String ConfigManager::toJson() {
    JsonDocument doc;
    doc["apiBaseUrl"] = config.apiBaseUrl;
    doc["mdnsHostname"] = config.mdnsHostname;
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
    
    if (!apiUrl || !hostname) {
        Serial.println("Missing required fields in JSON");
        return false;
    }
    
    // Validate and set
    if (!setApiBaseUrl(apiUrl) || !setMdnsHostname(hostname)) {
        return false;
    }
    
    return true;
}

// =============================================
// GLOBAL INSTANCE
// =============================================

ConfigManager configManager;
