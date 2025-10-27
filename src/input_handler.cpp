#include "input_handler.h"

// =============================================
// KEYPAD BUTTON CLASS IMPLEMENTATION
// =============================================

KeypadButton::KeypadButton(uint8_t buttonPin, uint16_t debounceMs) :
    pin(buttonPin), lastState(HIGH), currentState(HIGH), 
    lastDebounceTime(0), debounceDelay(debounceMs) {}

void KeypadButton::begin() {
    pinMode(pin, INPUT_PULLUP);
    currentState = digitalRead(pin);
    lastState = currentState;
    
    Serial.printf("Keypad Button initialized - Pin: %d, State: %s\n", 
                 pin, currentState ? "HIGH (released)" : "LOW (pressed)");
}

void KeypadButton::update() {
    readButton();
}

void KeypadButton::readButton() {
    int reading = digitalRead(pin);
    
    // Check if state changed
    if (reading != lastState) {
        lastDebounceTime = millis();
    }
    
    // Debounce the reading
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != currentState) {
            currentState = reading;
            Serial.printf("Button Pin %d changed state to: %s\n", 
                         pin, currentState ? "HIGH (released)" : "LOW (pressed)");
        }
    }
    
    lastState = reading;
}

bool KeypadButton::isPressed() {
    return currentState == LOW; // Button pressed when LOW
}

bool KeypadButton::wasReleased() {
    bool released = (lastState == LOW && currentState == HIGH);
    return released;
}

// =============================================
// ADDRESSABLE LED CLASS IMPLEMENTATION
// =============================================

AddressableLEDs::AddressableLEDs(uint8_t ledPin, uint16_t count) :
    ledStrip(nullptr), pin(ledPin), ledCount(count), lastUpdate(0) {
    // WS2812B uses NEO_RGB color order
    ledStrip = new Adafruit_NeoPixel(count, pin, NEO_RGB + NEO_KHZ800);
}

AddressableLEDs::~AddressableLEDs() {
    if (ledStrip) {
        delete ledStrip;
        ledStrip = nullptr;
    }
}

void AddressableLEDs::begin() {
    if (ledStrip) {
        ledStrip->begin();
        ledStrip->clear();
        ledStrip->show();
        Serial.printf("Addressable LEDs initialized - Pin: %d, Count: %d\n", pin, ledCount);
        
        // Test LED with different colors
        Serial.println("Testing LED colors...");
        
        // Test RED
        ledStrip->setPixelColor(0, ledStrip->Color(128, 0, 0));
        ledStrip->show();
        Serial.println("Test: RED (128, 0, 0)");
        delay(1000);
        
        // Test GREEN
        ledStrip->setPixelColor(0, ledStrip->Color(0, 128, 0));
        ledStrip->show();
        Serial.println("Test: GREEN (0, 128, 0)");
        delay(1000);
        
        // Test BLUE
        ledStrip->setPixelColor(0, ledStrip->Color(0, 0, 128));
        ledStrip->show();
        Serial.println("Test: BLUE (0, 0, 128)");
        delay(1000);
        
        // Test OFF
        ledStrip->clear();
        ledStrip->show();
        Serial.println("Test: OFF");
        delay(500);
    }
}

void AddressableLEDs::update() {
    if (ledStrip) {
        ledStrip->show();
    }
}

uint32_t AddressableLEDs::createColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void AddressableLEDs::setInstitutionLED(int institution, uint32_t ledColor) {
    if (!ledStrip) {
        Serial.println("ERROR: ledStrip is NULL!");
        return;
    }
    
    // Turn off all LEDs first
    ledStrip->clear();
    
    // Extract original RGB values from the color uint32_t
    // Color format: 0xRRGGBB where RR is at bits 16-23
    uint8_t r = ((ledColor >> 16) & 0xFF);
    uint8_t g = ((ledColor >> 8) & 0xFF);
    uint8_t b = (ledColor & 0xFF);
    
    Serial.printf("=== SETTING LED FOR INSTITUTION %d ===\n", institution);
    Serial.printf("Color: R=%d, G=%d, B=%d\n", r, g, b);
    
    // Use Adafruit NeoPixel Color() directly with RGB values
    // The Color() function will handle the correct bit order
    ledStrip->setPixelColor(0, ledStrip->Color(r, g, b));
    ledStrip->show();
    
        Serial.printf("LED set with RGB(%d,%d,%d) at 100%% brightness - command sent!\n", r, g, b);
}

void AddressableLEDs::turnOffAll() {
    if (ledStrip) {
        ledStrip->clear();
        ledStrip->show();
    }
}

void AddressableLEDs::blinkLED(int ledIndex, int times, int delayMs) {
    if (!ledStrip || ledIndex >= ledCount) return;
    
    uint32_t originalColor = ledStrip->getPixelColor(ledIndex);
    
    for (int i = 0; i < times; i++) {
        ledStrip->setPixelColor(ledIndex, createColor(255, 255, 255));
        ledStrip->show();
        delay(delayMs);
        
        ledStrip->setPixelColor(ledIndex, createColor(0, 0, 0));
        ledStrip->show();
        delay(delayMs);
    }
    
    ledStrip->setPixelColor(ledIndex, originalColor);
    ledStrip->show();
}

void AddressableLEDs::setAllLEDs(uint32_t color) {
    if (!ledStrip) return;
    
    for (int i = 0; i < ledCount; i++) {
        ledStrip->setPixelColor(i, color);
    }
    ledStrip->show();
}

void AddressableLEDs::showRainbow() {
    if (!ledStrip) return;
    
    static uint16_t hue = 0;
    
    for (int i = 0; i < ledCount; i++) {
        ledStrip->setPixelColor(i, ledStrip->ColorHSV((hue + i * 65536 / ledCount) & 0xFFFF));
    }
    
    hue = (hue + 256) % 65536;
    ledStrip->show();
}

void AddressableLEDs::showPulse(uint32_t color) {
    if (!ledStrip) return;
    
    static int8_t brightness = 0;
    static bool increasing = true;
    
    if (increasing) {
        brightness += 5;
        if (brightness >= 255) {
            brightness = 255;
            increasing = false;
        }
    } else {
        brightness -= 5;
        if (brightness <= 0) {
            brightness = 0;
            increasing = true;
        }
    }
    
    // Extract RGB
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Apply brightness
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    setAllLEDs((r << 16) | (g << 8) | b);
}

void AddressableLEDs::showBlink(uint32_t color, int times) {
    if (!ledStrip) return;
    
    for (int i = 0; i < times; i++) {
        setAllLEDs(color);
        delay(200);
        turnOffAll();
        delay(200);
    }
}

void AddressableLEDs::updateAnimation() {
    // Animation logic can be added here
    // Currently handled by individual animation methods
}

// =============================================
// INPUT HANDLER CLASS IMPLEMENTATION
// =============================================

InputHandler::InputHandler() : 
    button1(nullptr), button2(nullptr), button3(nullptr), button4(nullptr),
    ledStrip(nullptr), currentInstitution(2), lastInstitution(2), lastCheckTime(0) {}

InputHandler::~InputHandler() {
    if (button1) delete button1;
    if (button2) delete button2;
    if (button3) delete button3;
    if (button4) delete button4;
    if (ledStrip) delete ledStrip;
}

void InputHandler::begin() {
    initializeKeypad();
    initializeLEDs();
}

void InputHandler::initializeKeypad() {
    Serial.println("=== KEYPAD INITIALIZATION ===");
    
    button1 = new KeypadButton(KEYPAD_BUTTON_1_PIN);
    button2 = new KeypadButton(KEYPAD_BUTTON_2_PIN);
    button3 = new KeypadButton(KEYPAD_BUTTON_3_PIN);
    button4 = new KeypadButton(KEYPAD_BUTTON_4_PIN);
    
    button1->begin();
    button2->begin();
    button3->begin();
    button4->begin();
    
    // Set initial LED based on default institution
    currentInstitution = 1;
    lastInstitution = 1;
    
    Serial.println("Keypad buttons initialized successfully!");
    Serial.println("Button mapping:");
    Serial.println("  Button 1 (GPIO 19) -> Institution 1");
    Serial.println("  Button 2 (GPIO 20) -> Institution 2");
    Serial.println("  Button 3 (GPIO 21) -> Institution 3");
    Serial.println("  Button 4 (GPIO 47) -> Additional function");
    Serial.println("====================================");
}

void InputHandler::initializeLEDs() {
    Serial.println("Initializing Addressable LEDs...");
    ledStrip = new AddressableLEDs(LED_ADDRESSABLE_PIN, LED_ADDRESSABLE_COUNT);
    ledStrip->begin();
    
    // Set initial LED based on current institution
    setActiveInstitution(currentInstitution);
    
    Serial.println("Addressable LEDs initialized successfully!");
}

void InputHandler::update() {
    if (millis() - lastCheckTime >= 50) {  // Update every 50ms
        if (button1) button1->update();
        if (button2) button2->update();
        if (button3) button3->update();
        if (button4) button4->update();
        if (ledStrip) ledStrip->update();
        
        lastCheckTime = millis();
    }
}

int InputHandler::getCurrentInstitution() {
    if (button1 && button1->isPressed()) {
        currentInstitution = 1;
        Serial.println("Button 1 pressed - Institution 1 selected");
        setActiveInstitution(1);  // Update LED immediately
    } else if (button2 && button2->isPressed()) {
        currentInstitution = 2;
        Serial.println("Button 2 pressed - Institution 2 selected");
        setActiveInstitution(2);  // Update LED immediately
    } else if (button3 && button3->isPressed()) {
        currentInstitution = 3;
        Serial.println("Button 3 pressed - Institution 3 selected");
        setActiveInstitution(3);  // Update LED immediately
    } else if (button4 && button4->isPressed()) {
        currentInstitution = 4;
        Serial.println("Button 4 pressed - Institution 4 selected");
        setActiveInstitution(4);  // Update LED immediately
    }
    
    return currentInstitution;
}

bool InputHandler::hasInstitutionChanged() {
    int newInstitution = getCurrentInstitution();
    
    if (newInstitution != lastInstitution) {
        lastInstitution = newInstitution;
        Serial.printf("Institution changed to: %d\n", newInstitution);
        return true;
    }
    
    return false;
}

String InputHandler::getInstitutionName() {
    switch (currentInstitution) {
        case 1: return "INSTITUTION_1";
        case 2: return "INSTITUTION_2";
        case 3: return "INSTITUTION_3";
        case 4: return "INSTITUTION_4";
        default: return "UNKNOWN";
    }
}

void InputHandler::setActiveInstitution(int institution) {
    currentInstitution = institution;
    
    if (ledStrip) {
        uint32_t ledColor;
        switch (institution) {
            case 1:
                ledColor = AddressableLEDs::createColor(0, 0, 255);  // BLUE
                ledStrip->setInstitutionLED(institution, ledColor);
                Serial.println("LED set to RED (Institution 1)");
                break;
            case 2:
                ledColor = AddressableLEDs::createColor(0, 255, 0);  // GREEN
                ledStrip->setInstitutionLED(institution, ledColor);
                Serial.println("LED set to BLUE (Institution 2)");
                break;
            case 3:
                ledColor = AddressableLEDs::createColor(255, 0, 255);  // PURPLE (actual purple color)
                ledStrip->setInstitutionLED(institution, ledColor);
                Serial.println("LED set to PURPLE (Institution 3)");
                break;
            case 4:
                ledColor = AddressableLEDs::createColor(255, 0, 0);  // RED
                ledStrip->setInstitutionLED(institution, ledColor);
                Serial.println("LED set to GREEN (Institution 4)");
                break;
            default:
                ledStrip->turnOffAll();
                Serial.println("All LEDs turned off");
                break;
        }
    }
}

void InputHandler::blinkInstitutionLEDs() {
    if (ledStrip) {
        ledStrip->showBlink(AddressableLEDs::createColor(255, 255, 255), 3);
        // Restore institution color
        setActiveInstitution(currentInstitution);
    }
}

bool InputHandler::isButton4Pressed() {
    return (button4 && button4->isPressed());
}

void InputHandler::updateLEDs() {
    // LED update is handled in setActiveInstitution
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

String getInstitutionName(int institution) {
    switch (institution) {
        case 1: return "INSTITUTION_1";
        case 2: return "INSTITUTION_2";
        case 3: return "INSTITUTION_3";
        case 4: return "INSTITUTION_4";
        default: return "UNKNOWN";
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

InputHandler inputHandler;
