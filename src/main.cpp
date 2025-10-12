#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
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
#include "simple_led.h"

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

// RTOS Task Handles
TaskHandle_t stateMachineTaskHandle = NULL;
TaskHandle_t inputTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;

// RTOS Queues and Semaphores
QueueHandle_t inputQueue;
QueueHandle_t stateQueue;
SemaphoreHandle_t displayMutex;

// State-specific variables
bool waitingInputStarted = false;
bool waitingAutoSelectShown = false;
unsigned long waitingLastStateTransition = 0;
int institution = INSTITUTION_1; // Global institution variable

// Performance timing variables
unsigned long cardDetectionTime = 0;
unsigned long nfcReadStartTime = 0;
unsigned long nfcReadEndTime = 0;
unsigned long apiValidationStartTime = 0;
unsigned long apiValidationEndTime = 0;
unsigned long userInputStartTime = 0;
unsigned long userInputEndTime = 0;
unsigned long apiLoggingStartTime = 0;
unsigned long apiLoggingEndTime = 0;

// Input event structure
typedef struct
{
    int buttonPressed;
    unsigned long timestamp;
} InputEvent;

// State change event structure
typedef struct
{
    SystemState newState;
    unsigned long timestamp;
} StateEvent;

// =============================================
// FUNCTION PROTOTYPES
// =============================================

void setup();
void loop();

// RTOS Task Functions
void stateMachineTask(void *parameter);
void inputTask(void *parameter);
void displayTask(void *parameter);

// State Machine Functions
void handleStateMachine();
void transitionToState(SystemState newState);
void handleIdleState();
void handleValidatingState();
void handleWaitingForInputState();
void handleSubmittingState();
void handleDisplayResultState();
void handleOTAProgressState();
void handleOTACompleteState();
void handleErrorState();
bool initializeSystem();
void performSystemCheck();
void resetCardData();

// RTOS Helper Functions
void createTasks();
void deleteTasks();

// Performance Analysis Functions
void printPerformanceReport();
void resetPerformanceTimers();

// =============================================
// SETUP FUNCTION
// =============================================

void setup()
{
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("Sistem Pembaca Kartu Santri");
    Serial.println("========================================");

    // Initialize all components
    if (!initializeSystem())
    {
        Serial.println("System initialization failed!");
        display.showCustomMessage("Init Failed!", "Check Serial");
        while (true)
        {
            // Non-blocking error loop
            delay(1000);
        }
    }

    Serial.println("System initialized successfully!");

    // Create RTOS tasks
    createTasks();

    Serial.println("RTOS tasks created successfully!");
    transitionToState(IDLE);
}

// =============================================
// MAIN LOOP
// =============================================

void loop()
{
    // Update components that don't have dedicated tasks
    wifiHandler.update();
    buzzer.update();
    otaHandler.update();
    ledLoop(); // Update LED patterns

    // Check for OTA state triggers
    if (otaHandler.shouldTriggerOTAProgressState())
    {
        transitionToState(OTA_PROGRESS);
        otaHandler.resetOTAProgressTrigger();
    }

    if (otaHandler.shouldTriggerOTACompleteState())
    {
        transitionToState(OTA_COMPLETE);
        otaHandler.resetOTACompleteTrigger();
    }

    // Small delay to prevent excessive CPU usage
    delay(100);
}

// =============================================
// STATE MACHINE HANDLER
// =============================================

void handleStateMachine()
{
    unsigned long currentTime = millis();

    // State-specific handling
    switch (currentState)
    {
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

    case OTA_PROGRESS:
        handleOTAProgressState();
        break;

    case OTA_COMPLETE:
        handleOTACompleteState();
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

void handleIdleState()
{
    static unsigned long lastCardCheck = 0;

    // LED off in idle state
    setLEDState(LED_OFF);

    // Check for card periodically
    if (millis() - lastCardCheck >= 500)
    { // Check every 500ms
        if (nfcHandler.isCardPresent())
        {
            buzzer.playClick();
            currentCardUID = nfcHandler.getCardUID();

            if (currentCardUID.length() > 0)
            {
                cardDetectionTime = millis();
                Serial.println("========================================");
                Serial.println("PERFORMANCE ANALYSIS STARTED");
                Serial.println("========================================");
                Serial.print("Card detected: ");
                Serial.println(currentCardUID);
                Serial.print("Detection time: ");
                Serial.print(cardDetectionTime);
                Serial.println(" ms");
                setLEDState(LED_CARD_READING);
                transitionToState(VALIDATING);
            }
        }
        lastCardCheck = millis();
    }
}

void handleValidatingState()
{
    static bool validationStarted = false;

    // Reset validationStarted when entering this state
    if (millis() - stateStartTime < 100) {
        validationStarted = false;
    }

    if (!validationStarted)
    {
        display.showValidating();
        buzzer.playProcessingPulse();
        validationStarted = true;
        stateStartTime = millis();
    }

    // First, try to read santri data from card
    nfcReadStartTime = millis();
    if (nfcHandler.readSantriData(santriNama, santriInduk))
    {
        nfcReadEndTime = millis();
        Serial.print("Santri data read - Nama: ");
        Serial.print(santriNama);
        Serial.print(", Induk: ");
        Serial.println(santriInduk);
        Serial.print("NFC read time: ");
        Serial.print(nfcReadEndTime - nfcReadStartTime);
        Serial.println(" ms");

        // Now validate the card using santri ID from JSON data
        apiValidationStartTime = millis();
        if (apiClient.validateSantriCard(currentCardUID, santriInduk))
        {
            apiValidationEndTime = millis();
            Serial.print("API validation time: ");
            Serial.print(apiValidationEndTime - apiValidationStartTime);
            Serial.println(" ms");
            
            // Card is valid
            Serial.println("Card validation successful - transitioning to WAITING_FOR_INPUT");

            // Simplified transition without LED/buzzer to isolate the issue
            display.showUserInfo(santriNama);
            transitionToState(WAITING_FOR_INPUT);
        }
        else
        {
            apiValidationEndTime = millis();
            Serial.print("API validation time: ");
            Serial.print(apiValidationEndTime - apiValidationStartTime);
            Serial.println(" ms");
            
            // Card validation failed
            Serial.println("Card validation failed");
            setLEDState(LED_CARD_INVALID);
            display.showInvalidCard();
            buzzer.playError();
            transitionToState(DISPLAY_RESULT);
        }
    }
    else
    {
        Serial.println("Failed to read santri data from card");
        setLEDState(LED_CARD_INVALID);
        display.showInvalidCard();
        buzzer.playError();
        transitionToState(DISPLAY_RESULT);
    }
}

void handleWaitingForInputState()
{
    static unsigned long timeoutStartTime = 0;

    if (!waitingInputStarted)
    {
        // Start timing for user input
        userInputStartTime = millis();
        
        // Start scrolling the name if it's longer than 16 characters
        if (santriNama.length() > 16)
        {
            display.startScrolling(santriNama, 300); // 300ms delay between scrolls
        }
        else
        {
            display.showSelectActivity(santriNama);
        }
        waitingInputStarted = true;
        timeoutStartTime = millis(); // Start timeout timer
        Serial.println("Started input handling with timeout");
    }

    // Check for timeout (5 seconds) - simplified logic
    unsigned long elapsed = millis() - timeoutStartTime;
    if (elapsed >= 5000) { // 5 seconds timeout
        Serial.println("Timeout reached - auto-selecting button 1");

        // End timing for user input
        userInputEndTime = millis();

        // Set institution to INSTITUTION_1 for auto-select
        institution = INSTITUTION_1;
        Serial.println("INSTITUTION_1 auto-selected");

        // Stop scrolling
        display.stopScrolling();

        // Show auto-selection message briefly
        display.showCustomMessage("Auto Select", "Button 1");

        // Simulate button 1 press
        buzzer.playClick();
        Serial.println("Auto-selected button 1 - transitioning to SUBMITTING");

        transitionToState(SUBMITTING);
        return;
    }

    // Check for button press from queue
    InputEvent inputEvent;
    if (xQueueReceive(inputQueue, &inputEvent, 0) == pdTRUE)
    {
        Serial.printf("State Machine: Received button %d from queue\n", inputEvent.buttonPressed);
        
        // End timing for user input
        userInputEndTime = millis();
        
        buzzer.playClick();
        Serial.print("Button pressed: ");
        Serial.println(inputEvent.buttonPressed);

        // Set institution based on button pressed
        if (inputEvent.buttonPressed == 1) {
            institution = INSTITUTION_1;
            Serial.println("INSTITUTION_1 selected");
        } else if (inputEvent.buttonPressed == 2) {
            institution = INSTITUTION_2;
            Serial.println("INSTITUTION_2 selected");
        } else if (inputEvent.buttonPressed == 3) {
            institution = INSTITUTION_3;
            Serial.println("INSTITUTION_3 selected");
        }

        // Stop scrolling when button is pressed
        display.stopScrolling();

        transitionToState(SUBMITTING);
    }
}

void handleSubmittingState()
{
    static bool submissionStarted = false;

    // Reset submissionStarted when entering this state
    if (millis() - stateStartTime < 100) {
        submissionStarted = false;
    }

    if (!submissionStarted)
    {
        display.showProcessing();
        buzzer.playProcessingPulse();
        submissionStarted = true;
        stateStartTime = millis();
    }

    // Submit activity using the institution selected by user
    apiLoggingStartTime = millis();
    if (apiClient.logSantriActivity(santriInduk, institution))
    {
        apiLoggingEndTime = millis();
        Serial.print("API logging time: ");
        Serial.print(apiLoggingEndTime - apiLoggingStartTime);
        Serial.println(" ms");
        
        // Print complete performance report
        printPerformanceReport();
        
        setLEDState(LED_CARD_VALID);
        display.showSuccess();
        buzzer.playSuccess();
        Serial.println("Activity logged successfully");
        transitionToState(DISPLAY_RESULT);
    }
    else
    {
        apiLoggingEndTime = millis();
        Serial.print("API logging time: ");
        Serial.print(apiLoggingEndTime - apiLoggingStartTime);
        Serial.println(" ms");
        
        setLEDState(LED_SERVER_ERROR);
        display.showServerError();
        buzzer.playError();
        Serial.println("Failed to log activity");
        transitionToState(DISPLAY_RESULT);
    }
}

void handleDisplayResultState()
{
    // Wait for timeout or user interaction
    if (millis() - stateStartTime >= LCD_MESSAGE_DELAY)
    {
        resetCardData();
        transitionToState(IDLE);
    }
}

void handleErrorState()
{
    // Handle error state - could be WiFi errors, NFC errors, etc.
    if (millis() - stateStartTime >= LCD_MESSAGE_DELAY)
    {
        transitionToState(IDLE);
    }
}

void handleOTAProgressState()
{
    static unsigned long lastProgressUpdate = 0;

    // Set LED to OTA progress pattern
    setLEDState(LED_OTA_PROGRESS);

    // Update progress display periodically
    if (millis() - lastProgressUpdate >= 500)
    { // Update every 500ms
        if (otaHandler.isOTAInProgress())
        {
            unsigned int progress = otaHandler.getOTAProgress();
            unsigned int total = otaHandler.getOTATotal();

            // Calculate percentage safely (avoid division by zero)
            unsigned int percentage;
            if (total > 0 && progress > 0)
            {
                percentage = (progress * 100) / total;
            }
            else
            {
                percentage = 0;
            }

            // Update LCD with current progress
            String progressText = String(percentage) + "%";
            display.showCustomMessage(MSG_OTA_PROGRESS_1, progressText);

            Serial.printf("OTA Progress: %u%% (%u/%u bytes)\n", percentage, progress, total > 0 ? total : progress);
        }

        lastProgressUpdate = millis();
    }

    // Check if OTA is complete
    if (!otaHandler.isOTAInProgress())
    {
        if (otaHandler.isOTASuccess())
        {
            transitionToState(OTA_COMPLETE);
        }
        else
        {
            // OTA failed, go back to idle
            display.showCustomMessage("OTA Failed", "Check Serial");
            // Non-blocking delay - let state machine handle timing
            transitionToState(IDLE);
        }
    }
}

void handleOTACompleteState()
{
    // Show completion message for 3000ms before restart
    setLEDState(LED_CARD_VALID); // Green to indicate success
    display.showCustomMessage(MSG_OTA_COMPLETE_1, MSG_OTA_COMPLETE_2);

    if (millis() - stateStartTime >= 3000)
    {
        Serial.println("OTA complete delay finished - restarting...");
        ESP.restart();
    }
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

void transitionToState(SystemState newState)
{
    Serial.print("State transition: ");
    Serial.print(currentState);
    Serial.print(" -> ");
    Serial.println(newState);

    currentState = newState;
    stateStartTime = millis();
    lastActivity = millis();

    // Reset variables for specific states
    if (newState == WAITING_FOR_INPUT)
    {
        waitingInputStarted = false;
        waitingAutoSelectShown = false;
        waitingLastStateTransition = 0;
        Serial.println("Reset WAITING_FOR_INPUT variables");
    }
    
    // Reset static variables for other states
    if (newState == VALIDATING) {
        // Reset validation state
        Serial.println("Reset VALIDATING state");
    }
    
    if (newState == SUBMITTING) {
        // Reset submission state
        Serial.println("Reset SUBMITTING state");
    }

    // Send state change event to queue
    StateEvent stateEvent;
    stateEvent.newState = newState;
    stateEvent.timestamp = millis();
    xQueueSend(stateQueue, &stateEvent, 0);
}

bool initializeSystem()
{
    // Initialize simple LED first (shows booting animation)
    if (!simpleLED.init())
    {
        Serial.println("LED initialization failed!");
        return false;
    }
    setLEDState(LED_BOOTING);

    // Initialize display
    display.begin();

    // Initialize buzzer
    buzzer.begin();

    // Initialize input handler
    inputHandler.begin();

    // Initialize NFC
    if (!nfcHandler.begin())
    {
        Serial.println("NFC initialization failed!");
        return false;
    }

    // Initialize API client
    apiClient.begin();

    // Initialize WiFi
    if (!wifiHandler.begin())
    {
        Serial.println("WiFi initialization failed!");
        setLEDState(LED_WIFI_ERROR);
        return false;
    }

    // Test WiFi connection
    setLEDState(LED_WIFI_CONNECTING);
    if (!wifiHandler.connect())
    {
        Serial.println("WiFi connection failed!");
        display.showWiFiError();
        buzzer.playError();
        setLEDState(LED_WIFI_ERROR);
        // Non-blocking - WiFi will retry automatically
    }
    else
    {
        setLEDState(LED_WIFI_CONNECTED);
    }

    // Initialize OTA after WiFi is connected (run in background)
    if (wifiHandler.isWiFiConnected())
    {
        Serial.println("Starting OTA service...");
        if (otaHandler.begin())
        {
            Serial.println("OTA service started successfully");
            // OTA runs in background without disturbing card reading interface
        }
        else
        {
            Serial.println("Failed to start OTA service");
        }
    }

    return true;
}

void performSystemCheck()
{
    // Periodic system health check
    if (millis() - lastActivity >= 30000)
    { // Every 30 seconds
        if (!wifiHandler.isWiFiConnected())
        {
            Serial.println("WiFi disconnected, attempting reconnection...");
            wifiHandler.connect();
        }

        if (!apiClient.testConnection())
        {
            Serial.println("Server connection test failed");
        }

        // OTA status is available via web interface, no need to show on LCD

        lastActivity = millis();
    }
}

void resetCardData()
{
    currentCardUID = "";
    santriNama = "";
    santriInduk = "";
    resetPerformanceTimers();
}

// =============================================
// RTOS TASK IMPLEMENTATIONS
// =============================================

void createTasks()
{
    // Create queues
    inputQueue = xQueueCreate(10, sizeof(InputEvent));
    stateQueue = xQueueCreate(5, sizeof(StateEvent));
    displayMutex = xSemaphoreCreateMutex();

    if (inputQueue == NULL || stateQueue == NULL || displayMutex == NULL)
    {
        Serial.println("Failed to create RTOS objects!");
        return;
    }

    // Create tasks
    xTaskCreatePinnedToCore(
        stateMachineTask,        // Task function
        "StateMachine",          // Task name
        8192,                    // Stack size (increased from 4096)
        NULL,                    // Parameters
        2,                       // Priority
        &stateMachineTaskHandle, // Task handle
        1                        // Core (Core 1)
    );

    xTaskCreatePinnedToCore(
        inputTask,        // Task function
        "InputHandler",   // Task name
        4096,             // Stack size (increased from 2048)
        NULL,             // Parameters
        1,                // Priority
        &inputTaskHandle, // Task handle
        0                 // Core (Core 0)
    );

    xTaskCreatePinnedToCore(
        displayTask,        // Task function
        "DisplayManager",   // Task name
        2048,               // Stack size
        NULL,               // Parameters
        1,                  // Priority
        &displayTaskHandle, // Task handle
        0                   // Core (Core 0)
    );

    Serial.println("RTOS tasks created successfully!");
}

void deleteTasks()
{
    if (stateMachineTaskHandle != NULL)
    {
        vTaskDelete(stateMachineTaskHandle);
        stateMachineTaskHandle = NULL;
    }

    if (inputTaskHandle != NULL)
    {
        vTaskDelete(inputTaskHandle);
        inputTaskHandle = NULL;
    }

    if (displayTaskHandle != NULL)
    {
        vTaskDelete(displayTaskHandle);
        displayTaskHandle = NULL;
    }

    if (inputQueue != NULL)
    {
        vQueueDelete(inputQueue);
        inputQueue = NULL;
    }

    if (stateQueue != NULL)
    {
        vQueueDelete(stateQueue);
        stateQueue = NULL;
    }

    if (displayMutex != NULL)
    {
        vSemaphoreDelete(displayMutex);
        displayMutex = NULL;
    }
}

void stateMachineTask(void *parameter)
{
    Serial.println("State Machine Task started");

    while (true)
    {
        // Handle state machine
        handleStateMachine();

        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void inputTask(void *parameter)
{
    Serial.println("Input Task started");

    while (true)
    {
        // Check for button press
        int buttonPressed = inputHandler.checkButtonPressed();

        if (buttonPressed > 0)
        {
            Serial.printf("Input Task: Button %d pressed, sending to queue\n", buttonPressed);
            
            InputEvent inputEvent;
            inputEvent.buttonPressed = buttonPressed;
            inputEvent.timestamp = millis();

            // Send to queue (non-blocking)
            if (xQueueSend(inputQueue, &inputEvent, 0) != pdTRUE)
            {
                Serial.println("Input queue full - dropping event");
            }
            else
            {
                Serial.printf("Input Task: Event sent to queue successfully\n");
            }
        }

        // Small delay to reduce CPU usage
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void displayTask(void *parameter)
{
    Serial.println("Display Task started");

    while (true)
    {
        // Take mutex before updating display
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            display.update();
            xSemaphoreGive(displayMutex);
        }

        // Small delay
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================================
// PERFORMANCE ANALYSIS FUNCTIONS
// =============================================

void printPerformanceReport()
{
    Serial.println("========================================");
    Serial.println("PERFORMANCE ANALYSIS REPORT");
    Serial.println("========================================");
    
    unsigned long totalTime = apiLoggingEndTime - cardDetectionTime;
    
    Serial.print("Total time (card detection to logging): ");
    Serial.print(totalTime);
    Serial.println(" ms");
    
    Serial.print("NFC read time: ");
    Serial.print(nfcReadEndTime - nfcReadStartTime);
    Serial.println(" ms");
    
    Serial.print("API validation time: ");
    Serial.print(apiValidationEndTime - apiValidationStartTime);
    Serial.println(" ms");
    
    Serial.print("User input time: ");
    Serial.print(userInputEndTime - userInputStartTime);
    Serial.println(" ms");
    
    Serial.print("API logging time: ");
    Serial.print(apiLoggingEndTime - apiLoggingStartTime);
    Serial.println(" ms");
    
    // Calculate percentages
    if (totalTime > 0) {
        Serial.println("----------------------------------------");
        Serial.println("PERCENTAGE BREAKDOWN:");
        
        float nfcPercent = ((float)(nfcReadEndTime - nfcReadStartTime) / totalTime) * 100;
        float validationPercent = ((float)(apiValidationEndTime - apiValidationStartTime) / totalTime) * 100;
        float inputPercent = ((float)(userInputEndTime - userInputStartTime) / totalTime) * 100;
        float loggingPercent = ((float)(apiLoggingEndTime - apiLoggingStartTime) / totalTime) * 100;
        
        Serial.print("NFC read: ");
        Serial.print(nfcPercent, 1);
        Serial.println("%");
        
        Serial.print("API validation: ");
        Serial.print(validationPercent, 1);
        Serial.println("%");
        
        Serial.print("User input: ");
        Serial.print(inputPercent, 1);
        Serial.println("%");
        
        Serial.print("API logging: ");
        Serial.print(loggingPercent, 1);
        Serial.println("%");
    }
    
    Serial.println("========================================");
}

void resetPerformanceTimers()
{
    cardDetectionTime = 0;
    nfcReadStartTime = 0;
    nfcReadEndTime = 0;
    apiValidationStartTime = 0;
    apiValidationEndTime = 0;
    userInputStartTime = 0;
    userInputEndTime = 0;
    apiLoggingStartTime = 0;
    apiLoggingEndTime = 0;
}

// =============================================
// ADDITIONAL SETUP FOR COMPATIBILITY
// =============================================

// Note: PlatformIO handles the main() function automatically
// This code is compatible with both Arduino IDE and PlatformIO
