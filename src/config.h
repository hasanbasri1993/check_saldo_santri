#ifndef CONFIG_H
#define CONFIG_H

// =============================================
// PIN DEFINITIONS
// =============================================

// I2C Pins for LCD and PN532
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// LCD I2C Address
#define LCD_I2C_ADDR    0x27

// Keypad 1x4 Pins (4 buttons for institutions)
#define KEYPAD_BUTTON_3_PIN  19  // GPIO19 for Button 1 (Institution 1)
#define KEYPAD_BUTTON_4_PIN  20  // GPIO20 for Button 2 (Institution 2)
#define KEYPAD_BUTTON_1_PIN  21  // GPIO21 for Button 3 (Institution 3)
#define KEYPAD_BUTTON_2_PIN  47  // GPIO47 for Button 4 (Additional function)

// Addressable LED (WS2812B) Pin
#define LED_ADDRESSABLE_PIN  14  // GPIO14 for WS2812B LED strip
#define LED_ADDRESSABLE_COUNT 1  // Number of addressable LEDs (can be adjusted)

// Buzzer Pin
#define BUZZER_PIN      7  // GPIO19 for Buzzer

// LED Built-in RGB Pin (WeAct Studio ESP32-S3)
#define LED_PIN         48 // GPIO48 for built-in RGB LED (hardwired)
#define LED_COUNT       1  // Number of LEDs (built-in RGB LED)


#define VERSION "2.0.2"
// =============================================
// API CONFIGURATION
// =============================================

// Server Configuration
// Note: API_BASE_URL is now configurable via web interface and stored in EEPROM

// Note: Card validation now uses query parameters:
// GET /api/v1/santri/validate?id_card={uid}&id_santri={santri_id}&id_device={mac_address}

// Endpoints
#define VALIDATE_UID_ENDPOINT   "/check"
#define LOG_ACTIVITY_ENDPOINT   "/santri/visitor_santri/"

// OTA (Over-The-Air) Update Configuration:
// OTA runs in background after WiFi connection (no LCD display)
// Default OTA URL: http://<device_ip>:7779/update
// Username: admin
// Password: santri123
// Upload .bin file through web interface for wireless firmware updates

#define OTA_USERNAME "admin"
#define OTA_PASSWORD "santri123"

// =============================================
// TIMING CONSTANTS (milliseconds)
// =============================================

#define CARD_READ_TIMEOUT       5000    // 5 seconds timeout for card reading
#define BUTTON_DEBOUNCE_DELAY   50      // Debounce delay for buttons
#define LCD_MESSAGE_DELAY       3000    // 3 seconds for status messages
#define WIFI_CONNECTION_TIMEOUT 10000   // 10 seconds for WiFi connection

// =============================================
// AUDIO FEEDBACK FREQUENCIES
// =============================================

#define BEEP_FREQ           1000    // Short beep frequency
#define SUCCESS_FREQ        1500    // Success melody frequency
#define ERROR_FREQ          400     // Error tone frequency
#define CLICK_FREQ          800     // Button click frequency

#define BEEP_DURATION       100     // Short beep duration
#define LONG_BEEP_DURATION  500     // Long beep duration
#define PULSE_DURATION      200     // Pulse duration for processing

// =============================================
// STATE MACHINE DEFINITIONS
// =============================================

enum SystemState {
    IDLE,                   // Waiting for card
    VALIDATING,             // Validating card with server
    WAITING_FOR_INPUT,      // Card validated, waiting for button press
    SUBMITTING,             // Submitting activity to server
    DISPLAY_RESULT,         // Showing result and waiting for timeout
    OTA_PROGRESS,           // OTA update in progress
    OTA_COMPLETE,           // OTA update completed, waiting before reset
    ERROR_STATE             // Error state, waiting for retry
};

// =============================================
// INSTITUTION MAPPING
// =============================================

#define INSTITUTION_1   1   // Button 1 -> Institution 1
#define INSTITUTION_2   2   // Button 2 -> Institution 2
#define INSTITUTION_3   3   // Button 3 -> Institution 3

// =============================================
// LCD MESSAGES
// =============================================

#define MSG_IDLE_1              "Tempelkan Kartu"
#define MSG_IDLE_2              ""
#define MSG_VALIDATING_1        "Memvalidasi..."
#define MSG_VALIDATING_2        ""
#define MSG_SELECT_ACTIVITY_1   "Pilih Aktivitas:"
#define MSG_PROCESSING_1        "Mengirim Data..."
#define MSG_PROCESSING_2        ""
#define MSG_SUCCESS_1           "Data Tersimpan!"
#define MSG_SUCCESS_2           ""
#define MSG_INVALID_CARD_1      "Kartu Tidak Vali"
#define MSG_INVALID_CARD_2      "D"
#define MSG_SERVER_ERROR_1      "Error: Server"
#define MSG_SERVER_ERROR_2      "Down"
#define MSG_WIFI_ERROR_1        "Error: WiFi"
#define MSG_WIFI_ERROR_2        "Connection"
#define MSG_OTA_PROGRESS_1      "OTA Update..."
#define MSG_OTA_PROGRESS_2      "0%"
#define MSG_OTA_COMPLETE_1      "Update Complete!"
#define MSG_OTA_COMPLETE_2      "Restarting..."

// =============================================
// BUZZER PATTERNS
// =============================================

// Pattern definitions for different feedback types
#define PATTERN_CLICK       1   // Single short beep
#define PATTERN_SUCCESS     2   // Rising melody
#define PATTERN_ERROR       3   // Double beep
#define PATTERN_PROCESSING  4   // Slow pulse
#define PATTERN_WARNING     5   // Long descending tone

// =============================================
// LED PATTERNS DEFINITIONS
// =============================================

// LED States for different events
enum LEDState {
    LED_OFF,                // LED mati untuk mode idle
    LED_BOOTING,            // Breathing effect saat booting
    LED_WIFI_CONNECTING,    // Blink biru saat connecting WiFi
    LED_WIFI_CONNECTED,     // Hijau solid saat WiFi connected
    LED_WIFI_ERROR,         // Blink merah cepat saat WiFi error
    LED_OTA_PROGRESS,       // Rainbow cycle saat OTA
    LED_CARD_READING,       // Kuning saat membaca kartu
    LED_CARD_VALID,         // Hijau saat kartu valid
    LED_CARD_INVALID,       // Merah saat kartu invalid
    LED_SERVER_ERROR        // Merah blink saat server error
};

// LED Timing Constants
#define LED_BREATHING_DURATION    5000    // 5 seconds for booting
#define LED_WIFI_CONNECTED_DURATION 3000 // 3 seconds for WiFi connected
#define LED_BLINK_INTERVAL_CONNECTING 500 // 500ms for WiFi connecting
#define LED_BLINK_INTERVAL_ERROR 250     // 250ms for error states
#define LED_RAINBOW_SPEED        50      // Rainbow cycle speed

#endif // CONFIG_H
