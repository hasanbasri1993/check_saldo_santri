#include "display_manager.h"

// =============================================
// CLASS IMPLEMENTATION
// =============================================

DisplayManager::DisplayManager(uint8_t addr, uint8_t columns, uint8_t rows)
    : address(addr), cols(columns), rows(rows), isDisplayingMessage(false), messageStartTime(0), lcd(addr, columns, rows),
      isScrolling(false), scrollingText(""), scrollPosition(0), lastScrollTime(0), scrollDelay(500) {}

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
    delay(2); // LCD clear needs time to complete
}

void DisplayManager::showTwoLines(String line1, String line2) {
    clearDisplay();

    // Truncate to maximum length
    if (line1.length() > cols) {
        line1 = line1.substring(0, cols);
    }
    if (line2.length() > cols) {
        line2 = line2.substring(0, cols);
    }

    // Center text
    centerText(line1, cols);
    centerText(line2, cols);

    // Display both lines
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    currentLine1 = line1;
    currentLine2 = line2;
    
    delay(10); // Allow LCD to update
}

void DisplayManager::centerText(String& text, uint8_t width) {
    // First truncate if too long
    if (text.length() > width) {
        text = text.substring(0, width);
        return; // Already fits
    }
    
    // If shorter, center it with spaces
    if (text.length() < width) {
        uint8_t padding = (width - text.length()) / 2;
        String paddingStr = "";
        for (uint8_t i = 0; i < padding; i++) {
            paddingStr += " ";
        }
        text = paddingStr + text;
    }
    
    // Ensure final length matches width
    while (text.length() < width) {
        text += " ";
    }
    if (text.length() > width) {
        text = text.substring(0, width);
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
    // Update scrolling text if active
    if (isScrolling) {
        updateScrolling();
        return; // Don't clear scrolling messages
    }
    
    // Auto-clear temporary messages after delay
    if (isDisplayingMessage && (millis() - messageStartTime >= LCD_MESSAGE_DELAY)) {
        showIdleScreen();
    }
    
    // Periodically reinitialize LCD if it's been inactive for too long
    static unsigned long lastReinitCheck = 0;
    if (millis() - lastReinitCheck > 60000) { // Check every 60 seconds
        lastReinitCheck = millis();
        
        // Soft reset display
        lcd.init();
        lcd.backlight();
        
        // Restore current display
        if (!currentLine1.isEmpty() && !currentLine2.isEmpty()) {
            lcd.setCursor(0, 0);
            lcd.print(currentLine1);
            lcd.setCursor(0, 1);
            lcd.print(currentLine2);
        }
    }
}

void DisplayManager::showProgressBar(uint8_t percentage, uint8_t row) {
    if (percentage > 100) percentage = 100;
    if (cols == 0) return;  // Safety check

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

void DisplayManager::startScrolling(const String& text, unsigned long delayMs) {
    scrollingText = text;
    scrollPosition = 0;
    scrollDelay = delayMs;
    lastScrollTime = millis();
    isScrolling = true;
    
    // Stop any current message display
    isDisplayingMessage = false;
    
    Serial.println("Started scrolling: " + text);
}

void DisplayManager::stopScrolling() {
    isScrolling = false;
    scrollingText = "";
    scrollPosition = 0;
    Serial.println("Stopped scrolling");
}

void DisplayManager::updateScrolling() {
    if (!isScrolling) return;
    
    unsigned long currentTime = millis();
    
    // Check if it's time to scroll
    if (currentTime - lastScrollTime >= scrollDelay) {
        // Simplified scrolling to prevent crashes
        lcd.clear();
        
        // Simple scrolling - just show the text as-is for now
        lcd.setCursor(0, 0);
        if (scrollingText.length() > cols) {
            // Show first part of text
            lcd.print(scrollingText.substring(0, cols));
        } else {
            lcd.print(scrollingText);
        }
        
        // Display static second line
        lcd.setCursor(0, 1);
        lcd.print("Pilih aktivitas:");
        
        lastScrollTime = currentTime;
        
        // Stop scrolling after showing once to prevent infinite loop
        isScrolling = false;
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

DisplayManager display;
