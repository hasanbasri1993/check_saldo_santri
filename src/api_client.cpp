#include "api_client.h"
#include <WiFi.h>

// =============================================
// CLASS IMPLEMENTATION
// =============================================

APIClient::APIClient(const String& serverURL, const String& version) :
    baseURL(serverURL), apiVersion(version), requestTimeout(5000) {  // 5 second timeout
    lastResponseCode = 0;
    lastResponseBody = "";
}

void APIClient::begin() {
    // Initialize HTTP client if needed
    httpClient.setTimeout(requestTimeout);
}

bool APIClient::isReady() {
    return WiFi.status() == WL_CONNECTED;
}

bool APIClient::validateSantriCard(const String& uid) {
    if (!isReady()) {
        lastError = "WiFi not connected";
        return false;
    }

    String url = buildURL(VALIDATE_UID_ENDPOINT + uid);

    Serial.print("Validating card UID: ");
    Serial.println(uid);
    Serial.print("Request URL: ");
    Serial.println(url);

    int responseCode = sendGETRequest(url);

    if (responseCode == 200) {
        bool isValid;
        if (parseValidationResponse(lastResponseBody, isValid)) {
            Serial.print("Card validation result: ");
            Serial.println(isValid ? "Valid" : "Invalid");
            return isValid;
        } else {
            lastError = "Failed to parse validation response";
            return false;
        }
    } else {
        lastError = "HTTP request failed with code: " + String(responseCode);
        return false;
    }
}

bool APIClient::logSantriActivity(const String& memberID, int institution) {
    if (!isReady()) {
        lastError = "WiFi not connected";
        return false;
    }

    // Prepare form data
    String payload = "memberID=" + memberID +
                    "&counter=1" +
                    "&institution=" + String(institution);

    String url = buildURL(LOG_ACTIVITY_ENDPOINT);

    Serial.print("Logging activity for member: ");
    Serial.println(memberID);
    Serial.print("Institution: ");
    Serial.println(institution);
    Serial.print("Request URL: ");
    Serial.println(url);

    int responseCode = sendPOSTRequest(url, payload);

    if (responseCode == 200 || responseCode == 201) {
        bool success;
        if (parseActivityResponse(lastResponseBody, success)) {
            Serial.print("Activity log result: ");
            Serial.println(success ? "Success" : "Failed");
            return success;
        } else {
            lastError = "Failed to parse activity response";
            return false;
        }
    } else {
        lastError = "HTTP request failed with code: " + String(responseCode);
        return false;
    }
}

String APIClient::buildURL(const String& endpoint) {
    return baseURL +  endpoint;
}

bool APIClient::performRequest(const String& url, const String& method, const String& payload) {
    httpClient.setTimeout(requestTimeout);

    if (method == "GET") {
        return httpClient.begin(url);
    } else if (method == "POST") {
        httpClient.begin(url);
        httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
        return true;
    }

    return false;
}

int APIClient::sendGETRequest(const String& url) {
    if (!performRequest(url, "GET")) {
        lastError = "Failed to initialize HTTP GET request";
        return -1;
    }

    lastResponseCode = httpClient.GET();
    lastResponseBody = getResponseBody();

    httpClient.end();

    Serial.print("GET Response Code: ");
    Serial.println(lastResponseCode);
    Serial.print("Response Body: ");
    Serial.println(lastResponseBody);

    return lastResponseCode;
}

int APIClient::sendPOSTRequest(const String& url, const String& payload) {
    if (!performRequest(url, "POST")) {
        lastError = "Failed to initialize HTTP POST request";
        return -1;
    }

    lastResponseCode = httpClient.POST(payload);
    lastResponseBody = getResponseBody();

    httpClient.end();

    Serial.print("POST Response Code: ");
    Serial.println(lastResponseCode);
    Serial.print("Response Body: ");
    Serial.println(lastResponseBody);

    return lastResponseCode;
}

String APIClient::getResponseBody() {
    if (httpClient.getSize() > 0) {
        return httpClient.getString();
    }
    return "";
}

bool APIClient::parseValidationResponse(const String& response, bool& isValid) {
    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "JSON parsing failed: " + String(error.c_str());
        return false;
    }

    if (doc.containsKey("valid")) {
        isValid = doc["valid"].as<bool>();
        return true;
    }

    lastError = "Response missing 'valid' field";
    return false;
}

bool APIClient::parseActivityResponse(const String& response, bool& success) {
    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        lastError = "JSON parsing failed: " + String(error.c_str());
        return false;
    }

    // Check for success field or HTTP status
    if (doc.containsKey("success")) {
        success = doc["success"].as<bool>();
        return true;
    }

    // If no success field, assume success if we get a 200 response
    success = (lastResponseCode == 200 || lastResponseCode == 201);
    return true;
}

bool APIClient::testConnection() {
    if (!isReady()) {
        lastError = "WiFi not connected";
        return false;
    }

    String url = buildURL("/");
    int responseCode = sendGETRequest(url);

    return (responseCode > 0);  // Any response means server is reachable
}

String APIClient::getLastError() {
    return lastError;
}

void APIClient::setTimeout(unsigned long timeoutMs) {
    requestTimeout = timeoutMs;
    httpClient.setTimeout(requestTimeout);
}

int APIClient::getLastResponseCode() {
    return lastResponseCode;
}

String APIClient::getLastResponseBody() {
    return lastResponseBody;
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

bool isCardValid(const String& uid) {
    return apiClient.validateSantriCard(uid);
}

bool logActivity(const String& memberID, int institution) {
    return apiClient.logSantriActivity(memberID, institution);
}

bool pingServer() {
    return apiClient.testConnection();
}

// =============================================
// GLOBAL INSTANCE
// =============================================

APIClient apiClient;
