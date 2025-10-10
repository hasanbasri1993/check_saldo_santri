#include "wifi_handler.h"
#include <HTTPClient.h>

// =============================================
// CLASS IMPLEMENTATION
// =============================================

WiFiHandler::WiFiHandler() : isConnected(false), connectionStartTime(0), lastConnectionAttempt(0) {}

bool WiFiHandler::begin() {
    // Set WiFi mode to station
    WiFi.mode(WIFI_STA);

    // Set callback functions for WiFiManager events
    wifiManager.setAPCallback([this](WiFiManager* wifiMgr) {
        this->onWiFiManagerStarted();
    });

    // Set timeout for configuration portal
    wifiManager.setConfigPortalTimeout(180);  // 3 minutes

    // Set save config callback
    wifiManager.setSaveConfigCallback([this]() {
        this->onWiFiConnected();
    });

    // Try to connect or start configuration portal
    if (!wifiManager.autoConnect(apName, apPassword)) {
        Serial.println("Failed to connect and hit timeout");
        display.showWiFiError();
        buzzer.playError();
        return false;
    }

    return true;
}

bool WiFiHandler::connect() {
    if (isConnected) {
        return true;  // Already connected
    }

    connectionStartTime = millis();
    Serial.println("Attempting to connect to WiFi...");

    // Show connection progress on display
    showConnectionProgress();

    // Try to connect with timeout
    unsigned long startTime = millis();
    while (millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
        if (WiFi.status() == WL_CONNECTED) {
            isConnected = true;
            onWiFiConnected();
            return true;
        }
        delay(100);
    }

    // Connection failed
    isConnected = false;
    onWiFiDisconnected();
    return false;
}

bool WiFiHandler::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED && isConnected;
}

void WiFiHandler::disconnect() {
    WiFi.disconnect();
    isConnected = false;
    Serial.println("WiFi disconnected");
}

void WiFiHandler::resetSettings() {
    wifiManager.resetSettings();
    Serial.println("WiFiManager settings reset");
}

String WiFiHandler::getWiFiSSID() {
    if (isConnected) {
        return WiFi.SSID();
    }
    return "";
}

int32_t WiFiHandler::getWiFiRSSI() {
    if (isConnected) {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiHandler::getLocalIP() {
    if (isConnected) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

void WiFiHandler::update() {
    // Check connection status periodically
    if (isConnected && WiFi.status() != WL_CONNECTED) {
        isConnected = false;
        onWiFiDisconnected();
    }
}

void WiFiHandler::setAPCredentials(const char* name, const char* password) {
    apName = name;
    apPassword = password;
}

void WiFiHandler::setWiFiManagerTimeout(unsigned long timeoutMs) {
    wifiManager.setConfigPortalTimeout(timeoutMs / 1000);  // Convert to seconds
}

void WiFiHandler::printWiFiStatus() {
    Serial.println("=== WiFi Status ===");
    Serial.print("Connected: ");
    Serial.println(isConnected ? "Yes" : "No");
    Serial.print("SSID: ");
    Serial.println(getWiFiSSID());
    Serial.print("IP Address: ");
    Serial.println(getLocalIP());
    Serial.print("Signal Strength: ");
    Serial.print(getWiFiRSSI());
    Serial.println(" dBm");
}

unsigned long WiFiHandler::getConnectionUptime() {
    if (isConnected) {
        return millis() - connectionStartTime;
    }
    return 0;
}

// =============================================
// PRIVATE METHODS
// =============================================

void WiFiHandler::onWiFiConnected() {
    Serial.println("WiFi connected successfully!");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    display.showCustomMessage("WiFi Connected", getWiFiSSID());
    buzzer.playSuccess();

    isConnected = true;
    connectionStartTime = millis();
}

void WiFiHandler::onWiFiDisconnected() {
    Serial.println("WiFi disconnected!");

    display.showWiFiError();
    buzzer.playError();

    isConnected = false;
}

void WiFiHandler::onWiFiManagerStarted() {
    Serial.println("WiFiManager Access Point started");
    Serial.print("AP Name: ");
    Serial.println(apName);

    display.showCustomMessage("WiFi Setup Mode", "Connect to:");
    display.showCustomMessage(apName, "");
}

void WiFiHandler::showConnectionProgress() {
    static uint8_t progress = 0;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 500) {  // Update every 500ms
        String dots = "";
        for (uint8_t i = 0; i < progress; i++) {
            dots += ".";
        }

        display.showCustomMessage("Connecting WiFi", dots);
        progress = (progress + 1) % 4;  // Cycle through 0-3 dots
        lastUpdate = millis();
    }
}

bool WiFiHandler::isConnectionTimeout() {
    return (millis() - connectionStartTime) >= WIFI_CONNECTION_TIMEOUT;
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

bool testServerConnection(const char* serverURL) {
    if (!wifiHandler.isWiFiConnected()) {
        return false;
    }

    HTTPClient http;
    String testURL = String(serverURL);

    // Simple connectivity test - try to reach the server
    http.begin(testURL);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        http.end();
        return true;  // Server is reachable
    } else {
        Serial.print("Server connection failed, error: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}

bool attemptWiFiConnectionWithFeedback() {
    display.showCustomMessage("Connecting...", "");

    bool connected = wifiHandler.connect();

    if (connected) {
        display.showCustomMessage("Connected!", wifiHandler.getLocalIP());
        buzzer.playSuccess();
        delay(2000);  // Show success for 2 seconds
        return true;
    } else {
        display.showWiFiError();
        buzzer.playError();
        delay(3000);  // Show error for 3 seconds
        return false;
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

WiFiHandler wifiHandler;
