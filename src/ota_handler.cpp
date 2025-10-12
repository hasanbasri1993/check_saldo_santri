#include "ota_handler.h"
#include <WiFi.h>

// =============================================
// CLASS IMPLEMENTATION
// =============================================

// Static member definitions
const char* OTAHandler::otaUsername = "admin";
const char* OTAHandler::otaPassword = "santri123";
unsigned long ota_progress_millis = 0;

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

    ElegantOTA.setAuth(OTA_USERNAME, OTA_PASSWORD);
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(server);
    ElegantOTA.onProgress([this](size_t current, size_t final) {
        onOTAProgress(current, final);
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

    // Setup web server and OTA routes
    setupWebServer();

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
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - OTA</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}table{width:100%;border-collapse:collapse}";
        html += "th,td{padding:10px;border:1px solid#ddd;text-align:left}th{background:#f2f2f2}";
        html += ".btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer}";
        html += ".btn:hover{background:#0056b3}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+"- OTA Update</h1>";
        html += "<div style='max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)'>";
        html += "<table><tr><th>Information</th><th>Value</th></tr>";
        html += "<tr><td>Device Name</td><td>"+String(DEVICE_NAME)+"</td></tr>";
        html += "<tr><td>Firmware Version</td><td>"+String(VERSION)+"</td></tr>";
        html += "<tr><td>IP Address</td><td>" + WiFi.localIP().toString() + "</td></tr>";
        html += "<tr><td>MAC Address</td><td>" + WiFi.macAddress() + "</td></tr>";
        html += "<tr><td>Uptime</td><td>" + String(millis() / 1000) + " seconds</td></tr>";
        html += "<tr><td>Free Heap</td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
        html += "</table><br>";
        html += "<a href='/update'><button class='btn'>Go to OTA Update</button></a>";
        html += "</div></body></html>";

        request->send(200, "text/html", html);
    });

    // Device info endpoint
    server->on("/info", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String info = getDeviceInfo();
        request->send(200, "application/json", info);
    });
}

String OTAHandler::getDeviceInfo() {
    String info = "{";
    info += "\"device\":\"" + String(DEVICE_NAME) + "\",";
    info += "\"version\":\"" + String(VERSION) + "\",";
    info += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    info += "\"mac\":\"" + WiFi.macAddress() + "\",";
    info += "\"uptime\":" + String(millis() / 1000) + ",";
    info += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    info += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
    info += "\"wifi_rssi\":" + String(WiFi.RSSI()) + "\"";
    info += "}";
    return info;
}


void OTAHandler::update() {
    // OTA runs in background via AsyncWebServer
    // No need for continuous updates here

    ElegantOTA.loop();
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

void OTAHandler::onOTAProgress(size_t current, size_t final) {
    otaProgress = current;
    // Safe percentage calculation to avoid division by zero
    unsigned int percentage = 0;
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        Serial.printf("Progress: %u%%\n", (current * 100) / final);
    }
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

// =============================================
// GLOBAL INSTANCE
// =============================================

OTAHandler otaHandler;
