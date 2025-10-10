#include "input_handler.h"

// =============================================
// BUTTON CLASS IMPLEMENTATION
// =============================================

Button::Button(uint8_t buttonPin, uint16_t debounceMs) :
    pin(buttonPin), debounceDelay(debounceMs), lastState(HIGH), currentState(HIGH),
    lastDebounceTime(0), lastPressTime(0), isPressed(false) {}

void Button::begin() {
    pinMode(pin, INPUT_PULLUP);
    currentState = digitalRead(pin);
    lastState = currentState;
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
            currentState = reading;

            if (currentState == LOW) {  // Button pressed (assuming pullup)
                lastPressTime = millis();
                isPressed = true;
            } else {
                isPressed = false;
            }
        }
    }

    lastState = reading;
}

bool Button::isButtonPressed() {
    return (currentState == LOW);  // Active LOW for pullup configuration
}

bool Button::wasButtonPressed() {
    if (isPressed && (currentState == HIGH)) {
        isPressed = false;
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
    button1 = new Button(BUTTON_1_PIN);
    button2 = new Button(BUTTON_2_PIN);
    button3 = new Button(BUTTON_3_PIN);

    button1->begin();
    button2->begin();
    button3->begin();
}

void InputHandler::update() {
    if (millis() - lastButtonCheck >= 10) {  // Update every 10ms for responsiveness
        updateButtons();
        lastButtonCheck = millis();
    }
}

void InputHandler::updateButtons() {
    if (button1) button1->update();
    if (button2) button2->update();
    if (button3) button3->update();
}

int InputHandler::checkButtonPressed() {
    updateButtons();

    if (button1 && button1->wasButtonPressed()) {
        return 1;
    }
    if (button2 && button2->wasButtonPressed()) {
        return 2;
    }
    if (button3 && button3->wasButtonPressed()) {
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
