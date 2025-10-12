#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include "config.h"

// =============================================
// BUTTON CLASS
// =============================================

class Button {
private:
    uint8_t pin;
    bool lastState;
    bool currentState;
    unsigned long lastDebounceTime;
    unsigned long lastPressTime;
    bool isPressed;
    uint16_t debounceDelay;

public:
    Button(uint8_t buttonPin, uint16_t debounceMs = BUTTON_DEBOUNCE_DELAY);

    void begin();
    void update();
    bool isButtonPressed();
    bool wasButtonPressed();  // Returns true only once per press
    bool isButtonHeld(uint32_t holdTimeMs = 1000);  // Check if button is held for specified time
    unsigned long getLastPressDuration();  // Get duration of last press in milliseconds

private:
    void readState();
};

// =============================================
// INPUT HANDLER CLASS
// =============================================

class InputHandler {
private:
    Button* button1;
    Button* button2;
    Button* button3;

    unsigned long lastButtonCheck;

public:
    InputHandler();

    void begin();
    void update();

    // Check for button presses (returns button number 1-3, or 0 if none)
    int checkButtonPressed();

    // Check if any button is currently pressed
    bool isAnyButtonPressed();

    // Get specific button states
    bool isButton1Pressed();
    bool isButton2Pressed();
    bool isButton3Pressed();

    // Advanced button functions
    bool wasButton1Pressed();  // Returns true only once per press
    bool wasButton2Pressed();
    bool wasButton3Pressed();

    // Check for long press (held for more than 2 seconds)
    bool isButton1LongPressed();
    bool isButton2LongPressed();
    bool isButton3LongPressed();

private:
    void initializeButtons();
    void updateButtons();
    void debugButtonStates();
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern InputHandler inputHandler;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Non-blocking button check with timeout
int waitForButtonPress(unsigned long timeoutMs = 10000);  // Returns button number or 0 if timeout

#endif // INPUT_HANDLER_H
