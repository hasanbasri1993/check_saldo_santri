#include "display_manager.h"

// =============================================
// CLASS IMPLEMENTATION
// =============================================

DisplayManager::DisplayManager(uint8_t addr, uint8_t columns, uint8_t rows)
    : address(addr), cols(columns), rows(rows), isDisplayingMessage(false), messageStartTime(0), lcd(addr, columns, rows) {}

void DisplayManager::begin() {
    initLCD();
    showIdleScreen();
}

void DisplayManager::initLCD() {
    //Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void DisplayManager::clearDisplay() {
    lcd.clear();
}

void DisplayManager::showTwoLines(String line1, String line2) {
    clearDisplay();

    // Center and truncate text if necessary
    centerText(line1, cols);
    centerText(line2, cols);

    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    currentLine1 = line1;
    currentLine2 = line2;
}

void DisplayManager::centerText(String& text, uint8_t width) {
    if (text.length() > width) {
        text = text.substring(0, width);
    } else if (text.length() < width) {
        uint8_t padding = (width - text.length()) / 2;
        String paddingStr = "";
        for (uint8_t i = 0; i < padding; i++) {
            paddingStr += " ";
        }
        text = paddingStr + text;
    }
}

void DisplayManager::showIdleScreen() {
    showTwoLines(MSG_IDLE_1, MSG_IDLE_2);
    isDisplayingMessage = false;
}

void DisplayManager::showValidating() {
    showTwoLines(MSG_VALIDATING_1, MSG_VALIDATING_2);
    isDisplayingMessage = false;
}

void DisplayManager::showUserInfo(const String& name) {
    String line2 = name;
    centerText(line2, cols);
    showTwoLines("Kartu Valid", line2);
    isDisplayingMessage = false;
}

void DisplayManager::showSelectActivity(const String& name) {
    String line2 = name;
    centerText(line2, cols);
    showTwoLines(MSG_SELECT_ACTIVITY_1, line2);
    isDisplayingMessage = false;
}

void DisplayManager::showProcessing() {
    showTwoLines(MSG_PROCESSING_1, MSG_PROCESSING_2);
    isDisplayingMessage = false;
}

void DisplayManager::showSuccess() {
    showTwoLines(MSG_SUCCESS_1, MSG_SUCCESS_2);
    isDisplayingMessage = true;
    messageStartTime = millis();
}

void DisplayManager::showInvalidCard() {
    showTwoLines(MSG_INVALID_CARD_1, MSG_INVALID_CARD_2);
    isDisplayingMessage = true;
    messageStartTime = millis();
}

void DisplayManager::showServerError() {
    showTwoLines(MSG_SERVER_ERROR_1, MSG_SERVER_ERROR_2);
    isDisplayingMessage = true;
    messageStartTime = millis();
}

void DisplayManager::showWiFiError() {
    showTwoLines(MSG_WIFI_ERROR_1, MSG_WIFI_ERROR_2);
    isDisplayingMessage = true;
    messageStartTime = millis();
}

void DisplayManager::showMessage(String line1, String line2, int delayMs) {
    showTwoLines(line1, line2);
    isDisplayingMessage = true;
    messageStartTime = millis();

    // Note: The actual auto-clear will be handled in update() method
    // This allows for non-blocking operation
}

void DisplayManager::showCustomMessage(String line1, String line2) {
    showTwoLines(line1, line2);
    isDisplayingMessage = false; // Manual messages don't auto-clear
}

void DisplayManager::clear() {
    clearDisplay();
    isDisplayingMessage = false;
    currentLine1 = "";
    currentLine2 = "";
}

void DisplayManager::update() {
    // Auto-clear temporary messages after delay
    if (isDisplayingMessage && (millis() - messageStartTime >= LCD_MESSAGE_DELAY)) {
        showIdleScreen();
    }
}

void DisplayManager::showProgressBar(uint8_t percentage, uint8_t row) {
    if (percentage > 100) percentage = 100;

    uint8_t barLength = (cols * percentage) / 100;
    uint8_t emptyLength = cols - barLength;

    String bar = "";
    for (uint8_t i = 0; i < barLength; i++) {
        bar += "█";
    }
    for (uint8_t i = 0; i < emptyLength; i++) {
        bar += "░";
    }

    lcd.setCursor(0, row);
    lcd.print(bar);
}

void DisplayManager::scrollText(String text, uint8_t row, uint32_t delayMs) {
    if (text.length() <= cols) {
        lcd.setCursor(0, row);
        lcd.print(text);
        return;
    }

    // Simple scrolling implementation
    static uint32_t lastScrollTime = 0;
    static uint8_t scrollPosition = 0;

    if (millis() - lastScrollTime >= delayMs) {
        String displayText = text.substring(scrollPosition, scrollPosition + cols);

        if (displayText.length() < cols) {
            displayText += text.substring(0, cols - displayText.length());
        }

        lcd.setCursor(0, row);
        lcd.print(displayText);

        scrollPosition++;
        if (scrollPosition >= text.length()) {
            scrollPosition = 0;
        }

        lastScrollTime = millis();
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

DisplayManager display;
