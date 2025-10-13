#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include "config.h"

// =============================================
// TOGGLE SWITCH CLASS
// =============================================

class ToggleSwitch {
private:
    uint8_t pinA;
    uint8_t pinB;
    int lastPosition;
    int currentPosition;
    unsigned long lastDebounceTime;
    uint16_t debounceDelay;

public:
    ToggleSwitch(uint8_t switchPinA, uint8_t switchPinB, uint16_t debounceMs = 50);

    void begin();
    void update();
    int getCurrentPosition();  // Returns 1, 2, or 3
    bool hasPositionChanged();  // Returns true only once per position change
    String getPositionName();  // Returns "INST_1", "INST_2", or "INST_3"

private:
    void readPosition();
};

// =============================================
// INSTITUTION LED INDICATOR CLASS
// =============================================

class InstitutionLEDs {
private:
    uint8_t led1Pin;
    uint8_t led2Pin;
    uint8_t led3Pin;
    int currentActiveLED;

public:
    InstitutionLEDs(uint8_t pin1, uint8_t pin2, uint8_t pin3);

    void begin();
    void setActiveInstitution(int institution);  // 1, 2, or 3
    void turnOffAll();
    void blinkAll(int times = 3, int delayMs = 200);

private:
    void updateLEDs();
};

// =============================================
// INPUT HANDLER CLASS
// =============================================

class InputHandler {
private:
    ToggleSwitch* toggleSwitch;
    InstitutionLEDs* institutionLEDs;
    unsigned long lastCheckTime;

public:
    InputHandler();

    void begin();
    void update();

    // Get current institution from toggle switch
    int getCurrentInstitution();  // Returns 1, 2, or 3
    
    // Check if institution has changed
    bool hasInstitutionChanged();
    
    // Get institution name
    String getInstitutionName();

    // LED control
    void setActiveInstitution(int institution);
    void blinkInstitutionLEDs();

private:
    void initializeToggleSwitch();
    void initializeInstitutionLEDs();
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern InputHandler inputHandler;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Get institution name from number
String getInstitutionName(int institution);

#endif // INPUT_HANDLER_H