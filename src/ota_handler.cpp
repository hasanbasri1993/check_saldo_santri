#include "ota_handler.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config_manager.h"
#include <Preferences.h>

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
        MDNS.addServiceTxt("http", "tcp", "device", configManager.getDeviceName());
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
        MDNS.addService("http", "tcp", 7779);        
        Serial.println("mDNS service re-registered");
    } else {
        Serial.println("Failed to restart mDNS");
    }
}

void OTAHandler::setupWebServer() {
    if (!server) return;

    // Serve unified modern interface
    server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head><title>"+String(configManager.getDeviceName())+" - Control Panel</title>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>";
        html += "*{margin:0;padding:0;box-sizing:border-box}";
        html += "body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;color:#333}";
        html += ".container{max-width:1200px;margin:0 auto;padding:20px}";
        html += ".header{text-align:center;margin-bottom:30px;color:white}";
        html += ".header h1{font-size:2.5rem;margin-bottom:10px;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}";
        html += ".header p{font-size:1.1rem;opacity:0.9}";
        html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:30px}";
        html += ".card{background:rgba(255,255,255,0.95);border-radius:15px;padding:25px;box-shadow:0 8px 32px rgba(0,0,0,0.1);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.2);margin:15px;}";
        html += ".card h2{color:#4a5568;margin-bottom:20px;font-size:1.4rem;border-bottom:2px solid #e2e8f0;padding-bottom:10px}";
        html += ".info-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-bottom:20px}";
        html += ".info-item{display:flex;flex-direction:column}";
        html += ".info-label{font-weight:600;color:#718096;font-size:0.9rem;margin-bottom:5px}";
        html += ".info-value{color:#2d3748;font-size:1rem;word-break:break-all}";
        html += ".form-group{margin-bottom:20px}";
        html += ".form-group label{display:block;font-weight:600;color:#4a5568;margin-bottom:8px}";
        html += ".form-group input{width:100%;padding:12px 15px;border:2px solid #e2e8f0;border-radius:8px;font-size:1rem;transition:border-color 0.3s ease}";
        html += ".form-group input:focus{outline:none;border-color:#667eea;box-shadow:0 0 0 3px rgba(102,126,234,0.1)}";
        html += ".btn{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;padding:12px 24px;border:none;border-radius:8px;cursor:pointer;font-size:1rem;font-weight:600;transition:transform 0.2s ease,box-shadow 0.2s ease;text-decoration:none;display:inline-block;text-align:center}";
        html += ".btn:hover{transform:translateY(-2px);box-shadow:0 8px 25px rgba(102,126,234,0.3)}";
        html += ".btn-success{background:linear-gradient(135deg,#48bb78 0%,#38a169 100%)}";
        html += ".btn-success:hover{box-shadow:0 8px 25px rgba(72,187,120,0.3)}";
        html += ".btn-danger{background:linear-gradient(135deg,#f56565 0%,#e53e3e 100%)}";
        html += ".btn-danger:hover{box-shadow:0 8px 25px rgba(245,101,101,0.3)}";
        html += ".btn-warning{background:linear-gradient(135deg,#ed8936 0%,#dd6b20 100%)}";
        html += ".btn-warning:hover{box-shadow:0 8px 25px rgba(237,137,54,0.3)}";
        html += ".btn-info{background:linear-gradient(135deg,#4299e1 0%,#3182ce 100%)}";
        html += ".btn-info:hover{box-shadow:0 8px 25px rgba(66,153,225,0.3)}";
        html += ".btn-group{display:flex;gap:10px;flex-wrap:wrap;margin-top:20px}";
        html += ".alert{padding:15px;border-radius:8px;margin-bottom:20px;border-left:4px solid}";
        html += ".alert-success{background:#f0fff4;color:#22543d;border-left-color:#48bb78}";
        html += ".alert-error{background:#fed7d7;color:#742a2a;border-left-color:#f56565}";
        html += ".section{margin:15px;}";
        html += ".section h3{color:#4a5568;margin-bottom:15px;font-size:1.2rem}";
        html += ".status-indicator{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:8px}";
        html += ".status-online{background:#48bb78}";
        html += ".status-offline{background:#f56565}";
        html += "@media (max-width:768px){.grid{grid-template-columns:1fr}.header h1{font-size:2rem}.btn-group{flex-direction:column}.btn-group .btn{width:100%}}";
        html += "</style></head><body>";
        html += "<div class='container'>";
        html += "<div class='header'>";
        html += "<h1>"+String(configManager.getDeviceName())+"</h1>";
        html += "<p>Smart Card Reader Control Panel</p>";
        html += "</div>";
        html += "<div class='grid'>";
        html += "<div class='card'>";
        html += "<h2>Device Information</h2>";
        html += "<div class='info-grid'>";
        html += "<div class='info-item'><span class='info-label'>Device Name</span><span class='info-value'>"+String(configManager.getDeviceName())+"</span></div>";
        html += "<div class='info-item'><span class='info-label'>Firmware Version</span><span class='info-value'>"+String(VERSION)+"</span></div>";
        html += "<div class='info-item'><span class='info-label'>IP Address</span><span class='info-value'>"+WiFi.localIP().toString()+"</span></div>";
        html += "<div class='info-item'><span class='info-label'>MAC Address</span><span class='info-value'>"+WiFi.macAddress()+"</span></div>";
        html += "<div class='info-item'><span class='info-label'>Uptime</span><span class='info-value'>"+String(millis() / 1000)+" seconds</span></div>";
        html += "<div class='info-item'><span class='info-label'>Free Heap</span><span class='info-value'>"+String(ESP.getFreeHeap())+" bytes</span></div>";
        html += "</div>";
        html += "</div>";
        html += "<div class='card'>";
        html += "<h2>Network Status</h2>";
        html += "<div class='info-grid'>";
        html += "<div class='info-item'><span class='info-label'>WiFi SSID</span><span class='info-value'>"+WiFi.SSID()+"</span></div>";
        html += "<div class='info-item'><span class='info-label'>Signal Strength</span><span class='info-value'>"+String(WiFi.RSSI())+" dBm</span></div>";
        html += "<div class='info-item'><span class='info-label'>mDNS Hostname</span><span class='info-value'>"+String(configManager.getMdnsHostname())+".local</span></div>";
        html += "<div class='info-item'><span class='info-label'>Connection Status</span><span class='info-value'><span class='status-indicator status-online'></span>Connected</span></div>";
        html += "</div>";
        html += "</div>";
        html += "</div>";
        html += "<div class='card'>";
        html += "<h2>Configuration</h2>";
        html += "<form method='POST' action='/config'>";
        html += "<div class='form-group'><label for='deviceName'>Device Name</label><input type='text' id='deviceName' name='deviceName' value='"+String(configManager.getDeviceName())+"' required></div>";
        html += "<div class='form-group'><label for='apiUrl'>API Base URL</label><input type='url' id='apiUrl' name='apiUrl' value='"+String(configManager.getApiBaseUrl())+"' required></div>";
        html += "<div class='form-group'><label for='hostname'>mDNS Hostname</label><input type='text' id='hostname' name='hostname' value='"+String(configManager.getMdnsHostname())+"' required></div>";
        html += "<button type='submit' class='btn btn-success'>Save Configuration</button>";
        html += "</form>";
        html += "</div>";
        html += "<div class='card'>";
        html += "<h2>Authentication Settings</h2>";
        html += "<form method='POST' action='/auth-change'>";
        html += "<div class='form-group'><label for='newUsername'>Username</label><input type='text' id='newUsername' name='newUsername' value='"+String(authUsername)+"' required></div>";
        html += "<div class='form-group'><label for='newPassword'>New Password</label><input type='password' id='newPassword' name='newPassword' placeholder='Enter new password' required></div>";
        html += "<button type='submit' class='btn btn-success'>Change Authentication</button>";
        html += "</form>";
        html += "</div>";
        html += "<div class='card'>";
        html += "<h2>System Actions</h2>";
        html += "<div class='btn-group'>";
        html += "<a href='/update' class='btn'>OTA Update</a>";
        html += "<a href='/info' class='btn btn-info'>Device Info JSON</a>";
        html += "</div>";
        html += "<div class='section'>";
        html += "<h3>Danger Zone</h3>";
        html += "<div class='btn-group'>";
        html += "<form method='POST' action='/config/reset' onsubmit='return confirm(\"Are you sure you want to reset to defaults?\")' style='display:inline'>";
        html += "<button type='submit' class='btn btn-warning'>Reset to Defaults</button>";
        html += "</form>";
        html += "<form method='POST' action='/config/clear' onsubmit='return confirm(\"Are you sure you want to clear EEPROM? This will erase all stored data.\")' style='display:inline'>";
        html += "<button type='submit' class='btn btn-danger'>Clear EEPROM</button>";
        html += "</form>";
        html += "</div>";
        html += "</div>";
        html += "</div>";
        html += "</div>";
        html += "</div></body></html>";
        request->send(200, "text/html", html);
    });

    // Device info endpoint
    server->on("/info", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String info = getDeviceInfo();
        request->send(200, "application/json", info);
    });


    // Configuration save endpoint (for the unified interface)
    server->on("/config", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        // Safely read POST form parameters (body params)
        const AsyncWebParameter* pDeviceName = request->getParam("deviceName", true);
        const AsyncWebParameter* pApi = request->getParam("apiUrl", true);
        const AsyncWebParameter* pHost = request->getParam("hostname", true);
        
        bool success = true;
        String message = "";
        
        if (!pDeviceName || !pApi || !pHost) {
            success = false;
            message = "Missing parameters.";
        }
        
        String deviceName = success ? pDeviceName->value() : String("");
        String apiUrl = success ? pApi->value() : String("");
        String hostname = success ? pHost->value() : String("");
        deviceName.trim();
        apiUrl.trim();
        hostname.trim();
        if (success && (deviceName.length() == 0 || apiUrl.length() == 0 || hostname.length() == 0)) {
            success = false;
            message = "Empty parameters are not allowed.";
        }
        
        if (!configManager.setDeviceName(deviceName.c_str())) {
            success = false;
            message += "Invalid device name. ";
        }
        
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
        
        // Redirect back to main page with success/error message
        String redirectHtml = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'></head><body>";
        redirectHtml += "<div style='text-align:center;padding:50px;font-family:Arial,sans-serif'>";
        redirectHtml += "<h2>" + String(success ? "Success!" : "Error") + "</h2>";
        redirectHtml += "<p>" + message + "</p>";
        redirectHtml += "<p>Redirecting to main page...</p>";
        redirectHtml += "</div></body></html>";
        
        request->send(200, "text/html", redirectHtml);
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
        
        // Redirect back to main page with success/error message
        String redirectHtml = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'></head><body>";
        redirectHtml += "<div style='text-align:center;padding:50px;font-family:Arial,sans-serif'>";
        redirectHtml += "<h2>" + String(success ? "Success!" : "Error") + "</h2>";
        redirectHtml += "<p>" + message + "</p>";
        redirectHtml += "<p>Redirecting to main page...</p>";
        redirectHtml += "</div></body></html>";
        
        request->send(200, "text/html", redirectHtml);
    });

    // Configuration reset endpoint
    server->on("/config/reset", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        configManager.resetToDefaults();
        configManager.saveConfig();
        
        // Redirect back to main page
        String redirectHtml = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'></head><body>";
        redirectHtml += "<div style='text-align:center;padding:50px;font-family:Arial,sans-serif'>";
        redirectHtml += "<h2>Success!</h2>";
        redirectHtml += "<p>Configuration reset to defaults successfully! Device will restart.</p>";
        redirectHtml += "<p>Redirecting to main page...</p>";
        redirectHtml += "</div></body></html>";
        
        request->send(200, "text/html", redirectHtml);
        
        delay(2000);
        ESP.restart();
    });

    // Configuration clear EEPROM endpoint
    server->on("/config/clear", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        configManager.clearPreferences();
        
        // Redirect back to main page
        String redirectHtml = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'></head><body>";
        redirectHtml += "<div style='text-align:center;padding:50px;font-family:Arial,sans-serif'>";
        redirectHtml += "<h2>Success!</h2>";
        redirectHtml += "<p>Preferences cleared successfully! Device will restart.</p>";
        redirectHtml += "<p>Redirecting to main page...</p>";
        redirectHtml += "</div></body></html>";
        
        request->send(200, "text/html", redirectHtml);
        
        delay(2000);
        ESP.restart();
    });

    // Force mDNS restart endpoint
    server->on("/mdns-restart", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!authenticateRequest(request)) {
            request->requestAuthentication();
            return;
        }
        
        Serial.println("Force mDNS restart requested via web interface");
        restartMdns();
        
        // Redirect back to main page
        String redirectHtml = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/'></head><body>";
        redirectHtml += "<div style='text-align:center;padding:50px;font-family:Arial,sans-serif'>";
        redirectHtml += "<h2>Success!</h2>";
        redirectHtml += "<p>mDNS has been restarted with hostname: "+String(configManager.getMdnsHostname())+".local</p>";
        redirectHtml += "<p>Please wait a few minutes for the change to propagate to your router.</p>";
        redirectHtml += "<p><strong>For MikroTik users:</strong> You may need to clear mDNS cache:<br>";
        redirectHtml += "<code>/ip dns cache flush</code></p>";
        redirectHtml += "<p>Redirecting to main page...</p>";
        redirectHtml += "</div></body></html>";
        
        request->send(200, "text/html", redirectHtml);
    });
}

void OTAHandler::loadAuthCredentials() {
    Serial.println("Loading authentication credentials...");
    
    Preferences authPrefs;
    authPrefs.begin("auth_creds", false);
    
    // Load credentials with defaults
    String username = authPrefs.getString("username", "admin");
    String password = authPrefs.getString("password", "santri123");
    
    // Copy to static variables
    strncpy(authUsername, username.c_str(), sizeof(authUsername) - 1);
    authUsername[sizeof(authUsername) - 1] = '\0';
    
    strncpy(authPassword, password.c_str(), sizeof(authPassword) - 1);
    authPassword[sizeof(authPassword) - 1] = '\0';
    
    authPrefs.end();
    
    Serial.printf("Auth credentials loaded: %s\n", authUsername);
}

void OTAHandler::saveAuthCredentials() {
    Serial.println("Saving authentication credentials...");
    
    Preferences authPrefs;
    authPrefs.begin("auth_creds", false);
    
    authPrefs.putString("username", authUsername);
    authPrefs.putString("password", authPassword);
    
    authPrefs.end();
    
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
    info += "\"device\":\"" + String(configManager.getDeviceName()) + "\",";
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
