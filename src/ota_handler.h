#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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

    // Helper methods
    void setupWebServer();
    void setupOTARoutes();
    String getDeviceInfo();

public:
    OTAHandler();

    // Initialization
    bool begin(uint16_t port = 8080);
    void end();

    // OTA Management
    void handleOTA();
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

    // Status and information
    String getOTAStatus();
    uint16_t getOTAPort() const;

    // Update method (call in main loop)
    void update();

    // Security
    void setOTACredentials(const char* username, const char* password);
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
