#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <Update.h>
#include "config.h"

// =============================================
// OTA HANDLER CLASS
// =============================================

class OTAHandler {
private:
    AsyncWebServer* server;
    bool isRunning;
    unsigned long lastOTACheck;

    // OTA Progress tracking
    bool otaInProgress;
    unsigned int otaProgress;
    unsigned int otaTotal;
    bool otaSuccess;

    // State trigger flags
    bool shouldTriggerOTAProgress;
    bool shouldTriggerOTAComplete;

    // OTA Configuration
    static const char* otaUsername;
    static const char* otaPassword;
    
    // Authentication credentials (stored in EEPROM)
    char authUsername[32];
    char authPassword[32];

    // Helper methods
    void setupWebServer();
    String getDeviceInfo();
    
    // Authentication management
    void loadAuthCredentials();
    void saveAuthCredentials();
    bool authenticateRequest(AsyncWebServerRequest *request);
    String base64Decode(String input);

public:
    OTAHandler();

    // Initialization
    bool begin(uint16_t port = 8080);
    void end();
    void restartMdns();

    // OTA Management
    bool isOTARunning() const { return isRunning; }

    // OTA Progress callbacks (to be called from ElegantOTA)
    void onOTAStart(unsigned long fileSize = 0);
    void onOTAProgress(unsigned int progress, unsigned int total);
    void onOTAEnd(bool success);

    // OTA Progress getters
    bool isOTAInProgress() const { return otaInProgress; }
    unsigned int getOTAProgress() const { return otaProgress; }
    unsigned int getOTATotal() const { return otaTotal; }
    bool isOTASuccess() const { return otaSuccess; }

    // State trigger getters
    bool shouldTriggerOTAProgressState() const { return shouldTriggerOTAProgress; }
    bool shouldTriggerOTACompleteState() const { return shouldTriggerOTAComplete; }

    // State trigger resetters
    void resetOTAProgressTrigger() { shouldTriggerOTAProgress = false; }
    void resetOTACompleteTrigger() { shouldTriggerOTAComplete = false; }

    // Update method (call in main loop)
    void update();
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern OTAHandler otaHandler;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Quick OTA setup with default credentials
bool setupOTA(uint16_t port = 8080);

// Check if OTA is active
bool isOTAEnabled();

// Get OTA status for display
String getOTAInfo();

#endif // OTA_HANDLER_H
