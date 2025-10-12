#include "input_handler.h"

// =============================================
// BUTTON CLASS IMPLEMENTATION
// =============================================

Button::Button(uint8_t buttonPin, uint16_t debounceMs) :
    pin(buttonPin), debounceDelay(debounceMs), lastState(HIGH), currentState(HIGH),
    lastDebounceTime(0), lastPressTime(0), isPressed(false) {}

void Button::begin() {
    // Special configuration for GPIO 45 (boot button)
    if (pin == 45) {
        pinMode(pin, INPUT_PULLUP);
        Serial.println("GPIO 45 configured as INPUT_PULLUP for boot button");
    } else {
        pinMode(pin, INPUT_PULLUP);
    }
    
    currentState = digitalRead(pin);
    lastState = currentState;
    Serial.printf("Button on GPIO %d initialized - Initial state: %d\n", pin, currentState);
}

void Button::update() {
    readState();
}

void Button::readState() {
    bool reading = digitalRead(pin);

    if (reading != lastState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != currentState) {
            Serial.printf("Button GPIO %d: State changed from %d to %d\n", pin, currentState, reading);
            currentState = reading;

            if (currentState == LOW) {  // Button pressed (assuming pullup)
                lastPressTime = millis();
                isPressed = true;
                Serial.printf("Button GPIO %d: PRESSED (isPressed=true)\n", pin);
            } else {
                isPressed = false;
                Serial.printf("Button GPIO %d: RELEASED (isPressed=false)\n", pin);
            }
        }
    }

    lastState = reading;
}

bool Button::isButtonPressed() {
    return (currentState == LOW);  // Active LOW for pullup configuration
}

bool Button::wasButtonPressed() {
    if (isPressed && (currentState == LOW)) {  // Button is currently pressed
        isPressed = false;  // Reset flag to prevent multiple triggers
        return true;
    }
    return false;
}

bool Button::isButtonHeld(uint32_t holdTimeMs) {
    if (currentState == LOW) {
        return (millis() - lastPressTime) >= holdTimeMs;
    }
    return false;
}

unsigned long Button::getLastPressDuration() {
    if (currentState == HIGH && lastPressTime > 0) {
        return millis() - lastPressTime;
    }
    return 0;
}

// =============================================
// INPUT HANDLER CLASS IMPLEMENTATION
// =============================================

InputHandler::InputHandler() : button1(nullptr), button2(nullptr), button3(nullptr), lastButtonCheck(0) {}

void InputHandler::begin() {
    initializeButtons();
}

void InputHandler::initializeButtons() {
    Serial.println("Initializing buttons...");
    Serial.printf("Button 1 Pin: %d (GPIO 45 - Built-in Key)\n", BUTTON_1_PIN);
    Serial.printf("Button 2 Pin: %d (GPIO 4)\n", BUTTON_2_PIN);
    Serial.printf("Button 3 Pin: %d (GPIO 5)\n", BUTTON_3_PIN);
    
    button1 = new Button(BUTTON_1_PIN);
    button2 = new Button(BUTTON_2_PIN);
    button3 = new Button(BUTTON_3_PIN);

    button1->begin();
    button2->begin();
    button3->begin();
    
    Serial.println("Buttons initialized successfully!");
}

void InputHandler::debugButtonStates() {
    static unsigned long lastDebugTime = 0;
    
    // Debug every 5 seconds to reduce frequency
    if (millis() - lastDebugTime >= 5000) {
        Serial.printf("Button States - GPIO 45: %d, GPIO 4: %d, GPIO 5: %d\n", 
                     digitalRead(BUTTON_1_PIN), 
                     digitalRead(BUTTON_2_PIN), 
                     digitalRead(BUTTON_3_PIN));
        lastDebugTime = millis();
    }
}

void InputHandler::updateButtons() {
    if (button1) button1->update();
    if (button2) button2->update();
    if (button3) button3->update();
    
    // Debug button states less frequently
    // debugButtonStates();
}

int InputHandler::checkButtonPressed() {
    updateButtons();

    // Debug: Check individual button states
    if (button1 && button1->wasButtonPressed()) {
        Serial.println("Button 1 (GPIO 45) wasButtonPressed() returned true!");
        return 1;
    }
    if (button2 && button2->wasButtonPressed()) {
        Serial.println("Button 2 (GPIO 4) wasButtonPressed() returned true!");
        return 2;
    }
    if (button3 && button3->wasButtonPressed()) {
        Serial.println("Button 3 (GPIO 5) wasButtonPressed() returned true!");
        return 3;
    }

    return 0;  // No button pressed
}

bool InputHandler::isAnyButtonPressed() {
    updateButtons();
    return (isButton1Pressed() || isButton2Pressed() || isButton3Pressed());
}

bool InputHandler::isButton1Pressed() {
    return (button1 && button1->isButtonPressed());
}

bool InputHandler::isButton2Pressed() {
    return (button2 && button2->isButtonPressed());
}

bool InputHandler::isButton3Pressed() {
    return (button3 && button3->isButtonPressed());
}

bool InputHandler::wasButton1Pressed() {
    return (button1 && button1->wasButtonPressed());
}

bool InputHandler::wasButton2Pressed() {
    return (button2 && button2->wasButtonPressed());
}

bool InputHandler::wasButton3Pressed() {
    return (button3 && button3->wasButtonPressed());
}

bool InputHandler::isButton1LongPressed() {
    return (button1 && button1->isButtonHeld(2000));  // 2 second long press
}

bool InputHandler::isButton2LongPressed() {
    return (button2 && button2->isButtonHeld(2000));
}

bool InputHandler::isButton3LongPressed() {
    return (button3 && button3->isButtonHeld(2000));
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

int waitForButtonPress(unsigned long timeoutMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < timeoutMs) {
        int button = inputHandler.checkButtonPressed();
        if (button > 0) {
            return button;
        }
        delay(10);  // Small delay to prevent busy waiting
    }

    return 0;  // Timeout
}

// =============================================
// GLOBAL INSTANCE
// =============================================

InputHandler inputHandler;
