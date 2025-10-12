#include "ota_handler.h"
#include <WiFi.h>

// =============================================
// CLASS IMPLEMENTATION
// =============================================

// Static member definitions
const char* OTAHandler::otaUsername = "admin";
const char* OTAHandler::otaPassword = "santri123";

OTAHandler::OTAHandler() : server(nullptr), isRunning(false), lastOTACheck(0),
    otaInProgress(false), otaProgress(0), otaTotal(100), otaSuccess(false),
    shouldTriggerOTAProgress(false), shouldTriggerOTAComplete(false) {}

bool OTAHandler::begin(uint16_t port) {
    Serial.println("Starting OTA Web Server...");

    // Create web server
    server = new AsyncWebServer(port);

    if (!server) {
        Serial.println("Failed to create OTA web server!");
        return false;
    }

    // Setup web server and OTA routes
    setupWebServer();
    //setupOTARoutes();
    ElegantOTA.setAutoReboot(true);

    ElegantOTA.begin(server);

    ElegantOTA.onProgress([](size_t current, size_t final) {
        Serial.printf("Progress: %u%%\n", (current * 100) / final);
    });
    ElegantOTA.onEnd([](bool success) {
        if (success) {
          Serial.println("OTA update completed successfully.");
          unsigned long _reboot_request_millis = 0;

          if (millis() - _reboot_request_millis > 2000) {
              ESP.restart();
          }
    
        } else {
          Serial.println("OTA update failed.");
          // Add failure handling here.
        }
    });

    // Start server
    server->begin();
    isRunning = true;

    Serial.print("OTA Web Server started on port ");
    Serial.println(port);
    Serial.println("OTA URL: http://" + WiFi.localIP().toString() + ":" + String(port) + "/update");

    return true;
}

void OTAHandler::end() {
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    isRunning = false;
    Serial.println("OTA Web Server stopped");
}

void OTAHandler::setupWebServer() {
    if (!server) return;

    // Serve static files and information
    server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head><title>Santri Card Reader - OTA</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}table{width:100%;border-collapse:collapse}";
        html += "th,td{padding:10px;border:1px solid#ddd;text-align:left}th{background:#f2f2f2}";
        html += ".btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer}";
        html += ".btn:hover{background:#0056b3}</style></head><body>";
        html += "<h1>Santri Card Reader - OTA Update</h1>";
        html += "<div style='max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)'>";
        html += "<table><tr><th>Information</th><th>Value</th></tr>";
        html += "<tr><td>Device Name</td><td>Santri Card Reader</td></tr>";
        html += "<tr><td>Firmware Version</td><td>1.0.0</td></tr>";
        html += "<tr><td>IP Address</td><td>" + WiFi.localIP().toString() + "</td></tr>";
        html += "<tr><td>MAC Address</td><td>" + WiFi.macAddress() + "</td></tr>";
        html += "<tr><td>Uptime</td><td>" + String(millis() / 1000) + " seconds</td></tr>";
        html += "<tr><td>Free Heap</td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
        html += "</table><br>";
        html += "<a href='/upload'><button class='btn'>Go to OTA Update</button></a>";
        html += "</div></body></html>";

        request->send(200, "text/html", html);
    });

    // Device info endpoint
    server->on("/info", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String info = getDeviceInfo();
        request->send(200, "application/json", info);
    });
}

void OTAHandler::setupOTARoutes() {
    if (!server) return;

    // OTA Upload endpoint
    server->on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Serial.println("OTA Update initiated");

        // Check authentication
        if (!request->authenticate(otaUsername, otaPassword)) {
            return request->requestAuthentication();
        }

        // Get file size from Content-Length header
        String contentLength = request->header("Content-Length");
        unsigned long fileSize = 0;
        if (contentLength.length() > 0) {
            fileSize = contentLength.toInt();
            Serial.printf("File size from header: %lu bytes\n", fileSize);
        } else {
            Serial.println("No Content-Length header found, using estimation");
        }

        // Handle OTA upload
        if (Update.hasError()) {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Update failed");
            response->addHeader("Connection", "close");
            request->send(response);
            return;
        }

    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        // Handle file upload for OTA
        if (!index) {
            Serial.printf("OTA Update Start: %s\n", filename.c_str());

            // Check file extension
            if (!filename.endsWith(".bin")) {
                Serial.println("Only .bin files allowed");
                return;
            }

            // Start OTA update and trigger progress callback
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Serial.println("Cannot start OTA update");
                return;
            }

            // Initialize OTA progress tracking (fileSize already processed above)
            otaHandler.onOTAStart();
        }

        // Write data
        if (Update.write(data, len) != len) {
            Serial.println("Failed to write OTA data");
            return;
        }

        // Update progress
        unsigned int progress = index + len;

        otaHandler.onOTAProgress(progress, otaTotal);

        // Finish update
        if (final) {
            bool success = Update.end(true);
            Serial.printf("OTA Update %s: %u bytes\n", success ? "Success" : "Failed", index + len);

            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
                success ? "Update successful" : "Update failed");
            response->addHeader("Connection", "close");
            request->send(response);

            // Trigger OTA end callback
            otaHandler.onOTAEnd(success);

            if (success) {
                // Delay 3000ms before restart to show completion message
                delay(3000);
                ESP.restart();
            }
        }
    });

    // Add upload form endpoint
    server->on("/upload", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head><title>OTA Upload</title>";
        html += "<style>body{font-family:Arial;margin:20px}";
        html += "input[type=file]{margin:10px 0;padding:10px;border:1px solid#ccc}";
        html += "input[type=submit]{background:#007bff;color:white;padding:10px 20px;border:none;cursor:pointer}";
        html += "</style></head><body>";
        html += "<h2>OTA Firmware Upload</h2>";
        html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
        html += "<input type='file' name='update' accept='.bin'><br><br>";
        html += "<input type='submit' value='Upload'>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html);
    });

    Serial.println("OTA routes configured");
}

String OTAHandler::getDeviceInfo() {
    String info = "{";
    info += "\"device\":\"Santri Card Reader\",";
    info += "\"version\":\"1.0.0\",";
    info += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    info += "\"mac\":\"" + WiFi.macAddress() + "\",";
    info += "\"uptime\":" + String(millis() / 1000) + ",";
    info += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    info += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
    info += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    info += "\"ota_enabled\":true";
    info += "}";
    return info;
}

void OTAHandler::handleOTA() {
    // OTA is handled by ElegantOTA in the background
    // This method can be used for additional OTA logic if needed
}

String OTAHandler::getOTAStatus() {
    if (isRunning) {
        return "OTA Active - http://" + WiFi.localIP().toString() + ":" + String(getOTAPort()) + "/update";
    } else {
        return "OTA Disabled";
    }
}

uint16_t OTAHandler::getOTAPort() const {
    return 8080;  // Default OTA port
}

void OTAHandler::update() {
    // OTA runs in background via AsyncWebServer
    // No need for continuous updates here

    ElegantOTA.loop();
}

void OTAHandler::setOTACredentials(const char* username, const char* password) {
    // Note: This needs to be called before begin() to take effect
    Serial.println("OTA credentials updated (restart required)");
}

void OTAHandler::onOTAStart(unsigned long fileSize) {
    Serial.println("OTA Update started - showing progress on LCD");
    otaInProgress = true;
    otaProgress = 0;

    // Use actual file size if available, otherwise estimate
    if (fileSize > 0) {
        otaTotal = fileSize;
        Serial.printf("Using actual file size: %lu bytes\n", fileSize);
    } else {
        otaTotal = 100000;  // Fallback estimation ~100KB
        Serial.println("Using estimated file size: 100KB");
    }

    otaSuccess = false;

    // Set flag to trigger state machine
    shouldTriggerOTAProgress = true;
}

void OTAHandler::onOTAProgress(unsigned int progress, unsigned int total) {
    otaProgress = progress;
    // Don't update otaTotal here as we already have it from Content-Length header

    // Safe percentage calculation to avoid division by zero
    unsigned int percentage = 0;
    if (otaTotal > 0 && progress > 0) {
        percentage = (progress * 100) / otaTotal;
    } else if (progress > 0) {
        // Estimate when total is not yet known (fallback)
        percentage = (progress * 100) / 100000;  // Assume ~100KB file
        if (percentage > 95) percentage = 95;   // Cap at 95%
    }

    Serial.printf("OTA Progress: %u/%u bytes (%u%%)\n", progress, otaTotal > 0 ? otaTotal : progress, percentage);
}

void OTAHandler::onOTAEnd(bool success) {
    Serial.printf("OTA Update %s\n", success ? "successful" : "failed");
    otaInProgress = false;
    otaSuccess = success;

    if (success) {
        Serial.println("OTA completed successfully - will restart in 3 seconds");
        shouldTriggerOTAComplete = true;
    } else {
        Serial.println("OTA failed");
    }
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

bool setupOTA(uint16_t port) {
    return otaHandler.begin(port);
}

bool isOTAEnabled() {
    return otaHandler.isOTARunning();
}

String getOTAInfo() {
    return otaHandler.getOTAStatus();
}

// =============================================
// GLOBAL INSTANCE
// =============================================

OTAHandler otaHandler;
