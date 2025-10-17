#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "config_manager.h"
#include "display_manager.h"
#include "buzzer_feedback.h"

// =============================================
// WIFI HANDLER CLASS
// =============================================

class WiFiHandler {
private:
    WiFiManager wifiManager;
    bool isConnected;
    unsigned long connectionStartTime;
    unsigned long lastConnectionAttempt;

    // WiFi Manager Configuration
    const char* apName = "SantriCardReader";
    const char* apPassword = "santri123";

    // Callback functions for WiFi events
    void onWiFiConnected();
    void onWiFiDisconnected();
    void onWiFiManagerStarted();

    // Helper methods
    void showConnectionProgress();
    bool isConnectionTimeout();

public:
    WiFiHandler();

    // Initialization and connection
    bool begin();
    bool connect();
    bool isWiFiConnected();

    // Connection management
    void disconnect();
    void resetSettings();  // Reset WiFiManager settings

    // Status and information
    String getWiFiSSID();
    int32_t getWiFiRSSI();
    String getLocalIP();

    // Update method (call in main loop)
    void update();

    // Configuration methods
    void setAPCredentials(const char* name, const char* password);
    void setWiFiManagerTimeout(unsigned long timeoutMs);

    // Debug and monitoring
    void printWiFiStatus();
    unsigned long getConnectionUptime();
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern WiFiHandler wifiHandler;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Check if WiFi is available and server is reachable
bool testServerConnection(const char* serverURL = "http://192.168.87.83:7894");

// Non-blocking connection attempt with visual feedback
bool attemptWiFiConnectionWithFeedback();

#endif // WIFI_HANDLER_H
