#include <Arduino.h>
// #include <Wire.h>

// Include all our custom modules
#include "config.h"
#include "wifi_handler.h"
#include "display_manager.h"
#include "nfc_handler.h"
#include "api_client.h"
#include "input_handler.h"
#include "buzzer_feedback.h"
#include "ota_handler.h"

// =============================================
// GLOBAL VARIABLES
// =============================================

// State machine variables
SystemState currentState = IDLE;
unsigned long stateStartTime = 0;

// Card data variables
String currentCardUID = "";
String santriNama = "";
String santriInduk = "";

// Timing variables
unsigned long lastStateUpdate = 0;
unsigned long lastActivity = 0;

// =============================================
// FUNCTION PROTOTYPES
// =============================================

void setup();
void loop();
void handleStateMachine();
void transitionToState(SystemState newState);
void handleIdleState();
void handleValidatingState();
void handleWaitingForInputState();
void handleSubmittingState();
void handleDisplayResultState();
void handleErrorState();
bool initializeSystem();
void performSystemCheck();
void resetCardData();

// =============================================
// SETUP FUNCTION
// =============================================

void setup() {
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("Sistem Pembaca Kartu Santri");
    Serial.println("========================================");

    // Initialize all components
    if (!initializeSystem()) {
        Serial.println("System initialization failed!");
        display.showCustomMessage("Init Failed!", "Check Serial");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("System initialized successfully!");
    transitionToState(IDLE);
}

// =============================================
// MAIN LOOP
// =============================================

void loop() {
    // Update all components
    wifiHandler.update();
    display.update();
    inputHandler.update();
    buzzer.update();
    otaHandler.update();

    // Handle state machine
    handleStateMachine();

    // Small delay to prevent excessive CPU usage
    delay(10);
}

// =============================================
// STATE MACHINE HANDLER
// =============================================

void handleStateMachine() {
    unsigned long currentTime = millis();

    // State-specific handling
    switch (currentState) {
        case IDLE:
            handleIdleState();
            break;

        case VALIDATING:
            handleValidatingState();
            break;

        case WAITING_FOR_INPUT:
            handleWaitingForInputState();
            break;

        case SUBMITTING:
            handleSubmittingState();
            break;

        case DISPLAY_RESULT:
            handleDisplayResultState();
            break;

        case ERROR_STATE:
            handleErrorState();
            break;

        default:
            Serial.println("Unknown state!");
            transitionToState(IDLE);
            break;
    }
}

// =============================================
// STATE HANDLERS
// =============================================

void handleIdleState() {
    static unsigned long lastCardCheck = 0;

    // Check for card periodically
    if (millis() - lastCardCheck >= 500) {  // Check every 500ms
        if (nfcHandler.isCardPresent()) {
            buzzer.playClick();
            currentCardUID = nfcHandler.getCardUID();

            if (currentCardUID.length() > 0) {
                Serial.print("Card detected: ");
                Serial.println(currentCardUID);
                transitionToState(VALIDATING);
            }
        }
        lastCardCheck = millis();
    }
}

void handleValidatingState() {
    static bool validationStarted = false;

    if (!validationStarted) {
        display.showValidating();
        buzzer.playProcessingPulse();
        validationStarted = true;
        stateStartTime = millis();
    }

    // First, try to read santri data from card
    if (nfcHandler.readSantriData(santriNama, santriInduk)) {
        Serial.print("Santri data read - Nama: ");
        Serial.print(santriNama);
        Serial.print(", Induk: ");
        Serial.println(santriInduk);

        // Now validate the card using santri ID from JSON data
        if (apiClient.validateSantriCard(currentCardUID, santriInduk)) {
            // Card is valid
            display.showUserInfo(santriNama);
            transitionToState(WAITING_FOR_INPUT);
        } else {
            // Card validation failed
            Serial.println("Card validation failed");
            display.showInvalidCard();
            buzzer.playError();
            transitionToState(DISPLAY_RESULT);
        }
    } else {
        Serial.println("Failed to read santri data from card");
        display.showInvalidCard();
        buzzer.playError();
        transitionToState(DISPLAY_RESULT);
    }
}

void handleWaitingForInputState() {
    static bool inputStarted = false;

    if (!inputStarted) {
        display.showSelectActivity(santriNama);
        inputStarted = true;
        stateStartTime = millis();
    }

    // Check for button press
    int buttonPressed = inputHandler.checkButtonPressed();

    if (buttonPressed > 0) {
        buzzer.playClick();
        Serial.print("Button pressed: ");
        Serial.println(buttonPressed);

        transitionToState(SUBMITTING);
    }
}

void handleSubmittingState() {
    static bool submissionStarted = false;

    if (!submissionStarted) {
        display.showProcessing();
        buzzer.playProcessingPulse();
        submissionStarted = true;
        stateStartTime = millis();
    }

    // Submit activity (this is a simplified version - in real implementation,
    // you'd want to check which button was pressed and get that info)
    int institution = INSTITUTION_1;  // Default to institution 1

    if (apiClient.logSantriActivity(santriInduk, institution)) {
        display.showSuccess();
        buzzer.playSuccess();
        Serial.println("Activity logged successfully");
        transitionToState(DISPLAY_RESULT);
    } else {
        display.showServerError();
        buzzer.playError();
        Serial.println("Failed to log activity");
        transitionToState(DISPLAY_RESULT);
    }
}

void handleDisplayResultState() {
    // Wait for timeout or user interaction
    if (millis() - stateStartTime >= LCD_MESSAGE_DELAY) {
        resetCardData();
        transitionToState(IDLE);
    }
}

void handleErrorState() {
    // Handle error state - could be WiFi errors, NFC errors, etc.
    if (millis() - stateStartTime >= LCD_MESSAGE_DELAY) {
        transitionToState(IDLE);
    }
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

void transitionToState(SystemState newState) {
    Serial.print("State transition: ");
    Serial.print(currentState);
    Serial.print(" -> ");
    Serial.println(newState);

    currentState = newState;
    stateStartTime = millis();
    lastActivity = millis();
}

bool initializeSystem() {
    // Initialize display
    display.begin();

    // Initialize buzzer
    buzzer.begin();

    // Initialize input handler
    inputHandler.begin();

    // Initialize NFC
    if (!nfcHandler.begin()) {
        Serial.println("NFC initialization failed!");
        return false;
    }

    // Initialize API client
    apiClient.begin();

    // Initialize WiFi
    if (!wifiHandler.begin()) {
        Serial.println("WiFi initialization failed!");
        return false;
    }

    // Test WiFi connection
    if (!wifiHandler.connect()) {
        Serial.println("WiFi connection failed!");
        display.showWiFiError();
        buzzer.playError();
        delay(3000);
    }

    // Initialize OTA after WiFi is connected (run in background)
    if (wifiHandler.isWiFiConnected()) {
        Serial.println("Starting OTA service...");
        if (otaHandler.begin()) {
            Serial.println("OTA service started successfully");
            // OTA runs in background without disturbing card reading interface
        } else {
            Serial.println("Failed to start OTA service");
        }
    }

    return true;
}

void performSystemCheck() {
    // Periodic system health check
    if (millis() - lastActivity >= 30000) {  // Every 30 seconds
        if (!wifiHandler.isWiFiConnected()) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            wifiHandler.connect();
        }

        if (!apiClient.testConnection()) {
            Serial.println("Server connection test failed");
        }

        // OTA status is available via web interface, no need to show on LCD

        lastActivity = millis();
    }
}

void resetCardData() {
    currentCardUID = "";
    santriNama = "";
    santriInduk = "";
}

// =============================================
// ADDITIONAL SETUP FOR COMPATIBILITY
// =============================================

// Note: PlatformIO handles the main() function automatically
// This code is compatible with both Arduino IDE and PlatformIO
