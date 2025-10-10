#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "config.h"

// =============================================
// DISPLAY MANAGER CLASS
// =============================================

class DisplayManager {
private:
    LiquidCrystal_I2C lcd;
    uint8_t address;
    uint8_t cols;
    uint8_t rows;

    unsigned long messageStartTime;
    bool isDisplayingMessage;
    String currentLine1;
    String currentLine2;

    // Helper methods
    void clearDisplay();
    void showTwoLines(String line1, String line2);
    void centerText(String& text, uint8_t width);

public:
    // Constructor
    DisplayManager(uint8_t addr = LCD_I2C_ADDR, uint8_t columns = 16, uint8_t rows = 2);

    // Initialization
    void begin();
    void initLCD();

    // Screen display methods
    void showIdleScreen();
    void showValidating();
    void showUserInfo(const String& name);
    void showSelectActivity(const String& name);
    void showProcessing();
    void showSuccess();
    void showInvalidCard();
    void showServerError();
    void showWiFiError();

    // Generic message display with auto-clear
    void showMessage(String line1, String line2, int delayMs = LCD_MESSAGE_DELAY);

    // Manual message display (no auto-clear)
    void showCustomMessage(String line1, String line2);

    // Clear display
    void clear();

    // Update method (call in main loop)
    void update();

    // Getters
    bool isMessageActive() const { return isDisplayingMessage; }
    String getCurrentLine1() const { return currentLine1; }
    String getCurrentLine2() const { return currentLine2; }

    // Utility methods
    void showProgressBar(uint8_t percentage, uint8_t row = 1);
    void scrollText(String text, uint8_t row = 0, uint32_t delayMs = 250);
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern DisplayManager display;

#endif // DISPLAY_MANAGER_H
