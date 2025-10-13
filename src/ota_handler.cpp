#include "ota_handler.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config_manager.h"

// =============================================
// CLASS IMPLEMENTATION
// =============================================

// Static member definitions
const char* OTAHandler::otaUsername = "admin";
const char* OTAHandler::otaPassword = "santri123";
unsigned long ota_progress_millis = 0;

OTAHandler::OTAHandler() : server(nullptr), isRunning(false), lastOTACheck(0),
    otaInProgress(false), otaProgress(0), otaTotal(100), otaSuccess(false),
    shouldTriggerOTAProgress(false), shouldTriggerOTAComplete(false) {
    // Initialize default auth credentials
    strcpy(authUsername, "admin");
    strcpy(authPassword, "santri123");
}

bool OTAHandler::begin(uint16_t port) {
    Serial.println("Starting OTA Web Server...");

    // Load authentication credentials
    loadAuthCredentials();

    // Initialize mDNS
    const char* hostname = configManager.getMdnsHostname();
    Serial.printf("Starting mDNS with hostname: %s\n", hostname);
    Serial.printf("Hostname length: %d characters\n", strlen(hostname));
    Serial.printf("Hostname bytes: ");
    for (int i = 0; i < strlen(hostname); i++) {
        Serial.printf("%02X ", (uint8_t)hostname[i]);
    }
    Serial.println();
    
    if (!MDNS.begin(hostname)) {
        Serial.println("Error setting up mDNS");
    } else {
        Serial.printf("mDNS responder started: %s.local\n", hostname);
        MDNS.addService("http", "tcp", port);
        MDNS.addServiceTxt("http", "tcp", "device", DEVICE_NAME);
        MDNS.addServiceTxt("http", "tcp", "version", VERSION);
        MDNS.addServiceTxt("http", "tcp", "hostname", hostname);
        MDNS.addServiceTxt("http", "tcp", "mac", WiFi.macAddress().c_str());
        
        Serial.println("mDNS service registered");
    }

    // Create web server
    server = new AsyncWebServer(port);

    if (!server) {
        Serial.println("Failed to create OTA web server!");
        return false;
    }

    ElegantOTA.begin(server);
    ElegantOTA.onProgress([this](size_t current, size_t final) {
        onOTAProgress(current, final);
    });
    ElegantOTA.onEnd([](bool success) {
        if (success) {
          Serial.println("OTA update completed successfully.");
        } else {
          Serial.println("OTA update failed.");
          // Add failure handling here.
        }
    });

    // Setup web server and OTA routes
    setupWebServer();

    // Start server
    server->begin();

    ElegantOTA.setAuth(authUsername, authPassword);
    ElegantOTA.setAutoReboot(true);

    isRunning = true;

    Serial.print("OTA Web Server started on port ");
    Serial.println(port);
    Serial.println("OTA URL: http://" + WiFi.localIP().toString() + ":" + String(port) + "/update");
    Serial.printf("mDNS URL: http://%s.local:%d/update\n", configManager.getMdnsHostname(), port);
    Serial.printf("Device MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Device IP: %s\n", WiFi.localIP().toString().c_str());

    return true;
}

void OTAHandler::end() {
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    
    // Stop mDNS
    MDNS.end();
    
    isRunning = false;
    Serial.println("OTA Web Server stopped");
}

void OTAHandler::restartMdns() {
    Serial.println("Restarting mDNS with new hostname...");
    
    // Stop current mDNS completely
    MDNS.end();
    delay(500); // Give time for cleanup
    
    // Start with new hostname
    const char* hostname = configManager.getMdnsHostname();
    Serial.printf("Attempting to start mDNS with hostname: %s\n", hostname);
    
    if (MDNS.begin(hostname)) {
        Serial.printf("mDNS restarted successfully with hostname: %s.local\n", hostname);
        MDNS.addService("http", "tcp", 8080);
        MDNS.addServiceTxt("http", "tcp", "device", DEVICE_NAME);
        MDNS.addServiceTxt("http", "tcp", "version", VERSION);
        MDNS.addServiceTxt("http", "tcp", "hostname", hostname);
        MDNS.addServiceTxt("http", "tcp", "mac", WiFi.macAddress().c_str());
        
        Serial.println("mDNS service re-registered");
    } else {
        Serial.println("Failed to restart mDNS");
    }
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
        html += "<a href='/config'><button class='btn' style='background:#28a745;margin-left:10px'>Configuration</button></a>";
        html += "<a href='/mdns-status'><button class='btn' style='background:#17a2b8;margin-left:10px'>mDNS Status</button></a>";
        html += "<a href='/info'><button class='btn' style='background:#007bff;margin-left:10px'>Info</button></a>";
        html += "</div></body></html>";

        request->send(200, "text/html", html);
    });

    // Device info endpoint
    server->on("/info", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String info = getDeviceInfo();
        request->send(200, "application/json", info);
    });

    // Configuration page
    server->on("/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - Configuration</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += "form{display:flex;flex-direction:column;gap:15px}label{font-weight:bold;color:#333}";
        html += "input[type=text],input[type=url]{padding:10px;border:1px solid #ddd;border-radius:5px;font-size:16px}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px}";
        html += ".btn:hover{background:#0056b3}.btn-success{background:#28a745}.btn-success:hover{background:#218838}";
        html += ".btn-danger{background:#dc3545}.btn-danger:hover{background:#c82333}";
        html += ".alert{padding:10px;margin:10px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".alert-error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - Configuration</h1>";
        html += "<div class='form-container'>";
        html += "<form method='POST' action='/config'>";
        html += "<label for='apiUrl'>API Base URL:</label>";
        html += "<input type='url' id='apiUrl' name='apiUrl' value='"+String(configManager.getApiBaseUrl())+"' required>";
        html += "<label for='hostname'>mDNS Hostname:</label>";
        html += "<input type='text' id='hostname' name='hostname' value='"+String(configManager.getMdnsHostname())+"' required>";
        html += "<button type='submit' class='btn btn-success'>Save Configuration</button>";
        html += "</form>";
        html += "<br><hr><h3>Authentication Settings</h3>";
        html += "<form method='POST' action='/auth-change'>";
        html += "<label for='newUsername'>New Username:</label>";
        html += "<input type='text' id='newUsername' name='newUsername' value='"+String(authUsername)+"' required>";
        html += "<label for='newPassword'>New Password:</label>";
        html += "<input type='password' id='newPassword' name='newPassword' placeholder='Enter new password' required>";
        html += "<button type='submit' class='btn btn-success'>Change Authentication</button>";
        html += "</form>";
        html += "<br><form method='POST' action='/config/reset' onsubmit='return confirm(\"Are you sure you want to reset to defaults?\")'>";
        html += "<button type='submit' class='btn btn-danger'>Reset to Defaults</button>";
        html += "</form>";
        html += "<br><form method='POST' action='/config/clear' onsubmit='return confirm(\"Are you sure you want to clear EEPROM? This will erase all stored data.\")'>";
        html += "<button type='submit' class='btn btn-danger' style='background:#dc3545'>Clear EEPROM</button>";
        html += "</form>";
        html += "<br><a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        request->send(200, "text/html", html);
    });

    // Configuration save endpoint
    server->on("/config", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        String apiUrl = request->getParam("apiUrl")->value();
        String hostname = request->getParam("hostname")->value();
        
        bool success = true;
        String message = "";
        
        if (!configManager.setApiBaseUrl(apiUrl.c_str())) {
            success = false;
            message += "Invalid API URL. ";
        }
        
        if (!configManager.setMdnsHostname(hostname.c_str())) {
            success = false;
            message += "Invalid hostname. ";
        }
        
        if (success) {
            if (configManager.saveConfig()) {
                message = "Configuration saved successfully! Device will restart to apply mDNS changes.";
                Serial.println("Configuration updated via web interface");
                
                // Restart mDNS with new hostname before restart
                restartMdns();
            } else {
                success = false;
                message = "Failed to save configuration to EEPROM.";
            }
        }
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - Configuration</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".alert{padding:15px;margin:15px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".alert-error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - Configuration</h1>";
        html += "<div class='form-container'>";
        html += "<div class='alert " + String(success ? "alert-success" : "alert-error") + "'>" + message + "</div>";
        html += "<a href='/config'><button class='btn'>Back to Configuration</button></a>";
        html += "<a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
        
        if (success) {
            delay(2000);
            ESP.restart();
        }
    });

    // Authentication change endpoint
    server->on("/auth-change", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        String newUsername = request->getParam("newUsername")->value();
        String newPassword = request->getParam("newPassword")->value();
        
        bool success = true;
        String message = "";
        
        // Validate new credentials
        if (newUsername.length() < 3 || newUsername.length() > 31) {
            success = false;
            message += "Username must be 3-31 characters. ";
        }
        
        if (newPassword.length() < 3 || newPassword.length() > 31) {
            success = false;
            message += "Password must be 3-31 characters. ";
        }
        
        if (success) {
            strcpy(authUsername, newUsername.c_str());
            strcpy(authPassword, newPassword.c_str());
            saveAuthCredentials();
            
            // Update ElegantOTA credentials
            ElegantOTA.setAuth(authUsername, authPassword);
            
            message = "Authentication credentials updated successfully!";
            Serial.printf("Auth credentials changed to: %s\n", authUsername);
        }
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - Auth Changed</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".alert{padding:15px;margin:15px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".alert-error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - Authentication Changed</h1>";
        html += "<div class='form-container'>";
        html += "<div class='alert " + String(success ? "alert-success" : "alert-error") + "'>" + message + "</div>";
        html += "<a href='/config'><button class='btn'>Back to Configuration</button></a>";
        html += "<a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
    });

    // Configuration reset endpoint
    server->on("/config/reset", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        configManager.resetToDefaults();
        configManager.saveConfig();
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - Configuration Reset</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".alert{padding:15px;margin:15px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - Configuration Reset</h1>";
        html += "<div class='form-container'>";
        html += "<div class='alert alert-success'>Configuration reset to defaults successfully! Device will restart.</div>";
        html += "<a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
        
        delay(2000);
        ESP.restart();
    });

    // Configuration clear EEPROM endpoint
    server->on("/config/clear", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        configManager.clearEEPROM();
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - EEPROM Cleared</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".alert{padding:15px;margin:15px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - EEPROM Cleared</h1>";
        html += "<div class='form-container'>";
        html += "<div class='alert alert-success'>EEPROM cleared successfully! Device will restart.</div>";
        html += "<a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
        
        delay(2000);
        ESP.restart();
    });

    // mDNS status endpoint
    server->on("/mdns-status", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - mDNS Status</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".status{padding:10px;margin:10px 0;border-radius:5px;background:#f8f9fa;border-left:4px solid #007bff}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - mDNS Status</h1>";
        html += "<div class='form-container'>";
        html += "<div class='status'><strong>Current mDNS Hostname:</strong> "+String(configManager.getMdnsHostname())+".local</div>";
        html += "<div class='status'><strong>Device MAC:</strong> "+WiFi.macAddress()+"</div>";
        html += "<div class='status'><strong>Device IP:</strong> "+WiFi.localIP().toString()+"</div>";
        html += "<div class='status'><strong>WiFi SSID:</strong> "+WiFi.SSID()+"</div>";
        html += "<div class='status'><strong>RSSI:</strong> "+String(WiFi.RSSI())+" dBm</div>";
        html += "<br><a href='/config'><button class='btn'>Change Configuration</button></a>";
        html += "<br><form method='POST' action='/mdns-restart' onsubmit='return confirm(\"Are you sure you want to restart mDNS?\")'>";
        html += "<button type='submit' class='btn' style='background:#ffc107;color:#000'>Restart mDNS</button>";
        html += "</form>";
        html += "<br><a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
    });

    // Force mDNS restart endpoint
    server->on("/mdns-restart", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        Serial.println("Force mDNS restart requested via web interface");
        restartMdns();
        
        String html = "<!DOCTYPE html><html><head><title>"+String(DEVICE_NAME)+" - mDNS Restarted</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += "h1{color:#333;text-align:center}.form-container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
        html += ".alert{padding:15px;margin:15px 0;border-radius:5px}.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
        html += ".btn{background:#007bff;color:white;padding:12px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;text-decoration:none;display:inline-block}</style></head><body>";
        html += "<h1>"+String(DEVICE_NAME)+" - mDNS Restarted</h1>";
        html += "<div class='form-container'>";
        html += "<div class='alert alert-success'>mDNS has been restarted with hostname: "+String(configManager.getMdnsHostname())+".local</div>";
        html += "<div class='alert alert-success'>Please wait a few minutes for the change to propagate to your router.</div>";
        html += "<div class='alert alert-success'><strong>For MikroTik users:</strong> You may need to clear mDNS cache:<br>";
        html += "<code>/ip dns cache flush</code><br>";
        html += "Or restart mDNS service:<br>";
        html += "<code>/ip dns set allow-remote-requests=no<br>/ip dns set allow-remote-requests=yes</code></div>";
        html += "<br><a href='/mdns-status'><button class='btn'>Check mDNS Status</button></a>";
        html += "<br><a href='/'><button class='btn'>Back to Main</button></a>";
        html += "</div></body></html>";
        
        request->send(200, "text/html", html);
    });
}

void OTAHandler::loadAuthCredentials() {
    Serial.println("Loading authentication credentials...");
    
    // Try to load from EEPROM (offset 200 to avoid conflict with config)
    EEPROM.begin(512);
    char tempUsername[32];
    char tempPassword[32];
    memset(tempUsername, 0, sizeof(tempUsername));
    memset(tempPassword, 0, sizeof(tempPassword));
    
    EEPROM.get(200, tempUsername);
    EEPROM.get(232, tempPassword);
    
    // Validate loaded credentials
    if (tempUsername[0] != '\0' && tempPassword[0] != '\0' &&
        strnlen(tempUsername, sizeof(tempUsername)) < sizeof(tempUsername) &&
        strnlen(tempPassword, sizeof(tempPassword)) < sizeof(tempPassword)) {
        strcpy(authUsername, tempUsername);
        strcpy(authPassword, tempPassword);
        Serial.printf("Auth credentials loaded: %s\n", authUsername);
    } else {
        Serial.println("Using default auth credentials");
        saveAuthCredentials(); // Save defaults to EEPROM
    }
}

void OTAHandler::saveAuthCredentials() {
    Serial.println("Saving authentication credentials...");
    
    EEPROM.put(200, authUsername);
    EEPROM.put(232, authPassword);
    EEPROM.commit();
    
    Serial.printf("Auth credentials saved: %s\n", authUsername);
}

bool OTAHandler::authenticateRequest(AsyncWebServerRequest *request) {
    // Use AsyncWebServer's built-in Basic Auth check
    return request->authenticate(authUsername, authPassword);
}

String OTAHandler::base64Decode(String input) {
    String output = "";
    int inputLen = input.length();
    
    for (int i = 0; i < inputLen; i += 4) {
        uint32_t value = 0;
        
        for (int j = 0; j < 4; j++) {
            char c = input.charAt(i + j);
            if (c >= 'A' && c <= 'Z') value |= (c - 'A') << (18 - j * 6);
            else if (c >= 'a' && c <= 'z') value |= (c - 'a' + 26) << (18 - j * 6);
            else if (c >= '0' && c <= '9') value |= (c - '0' + 52) << (18 - j * 6);
            else if (c == '+') value |= 62 << (18 - j * 6);
            else if (c == '/') value |= 63 << (18 - j * 6);
            else if (c == '=') break;
        }
        
        output += char((value >> 16) & 0xFF);
        if (i + 2 < inputLen && input.charAt(i + 2) != '=') {
            output += char((value >> 8) & 0xFF);
        }
        if (i + 3 < inputLen && input.charAt(i + 3) != '=') {
            output += char(value & 0xFF);
        }
    }
    
    return output;
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
    info += "\"wifi_rssi\":" + String(WiFi.RSSI());
    info += "}";
    return info;
}


void OTAHandler::update() {
    // OTA runs in background via AsyncWebServer
    // No need for continuous updates here

    ElegantOTA.loop();
    
    // mDNS is handled automatically by ESP32mDNS library
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
