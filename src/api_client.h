#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// =============================================
// API CLIENT CLASS
// =============================================

class APIClient {
private:
    HTTPClient httpClient;
    String baseURL;
    String apiVersion;
    unsigned long requestTimeout;

    // Helper methods
    String buildURL(const String& endpoint);
    bool performRequest(const String& url, const String& method, const String& payload = "");
    String getResponseBody();

    // Request/Response handling
    int sendGETRequest(const String& url);
    int sendPOSTRequest(const String& url, const String& payload);

    // Response parsing
    bool parseValidationResponse(const String& response, bool& isValid);
    bool parseActivityResponse(const String& response, bool& success);

public:
    APIClient(const String& serverURL = API_BASE_URL, const String& version = API_VERSION);

    // Initialization
    void begin();
    bool isReady();

    // Card validation
    bool validateSantriCard(const String& uid);

    // Activity logging
    bool logSantriActivity(const String& memberID, int institution);

    // Utility methods
    bool testConnection();
    String getLastError();
    void setTimeout(unsigned long timeoutMs);

    // Response data access (for debugging)
    int getLastResponseCode();
    String getLastResponseBody();

private:
    String lastError;
    int lastResponseCode;
    String lastResponseBody;
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern APIClient apiClient;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Quick validation check
bool isCardValid(const String& uid);

// Quick activity logging
bool logActivity(const String& memberID, int institution);

// Check server connectivity
bool pingServer();

#endif // API_CLIENT_H
