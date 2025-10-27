#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// =============================================
// KEYPAD BUTTON CLASS
// =============================================

class KeypadButton {
private:
    uint8_t pin;
    int lastState;
    int currentState;
    unsigned long lastDebounceTime;
    uint16_t debounceDelay;

public:
    KeypadButton(uint8_t buttonPin, uint16_t debounceMs = 50);
    
    void begin();
    void update();
    bool isPressed();  // Returns true when button is pressed
    bool wasReleased(); // Returns true only once when button is released
    
private:
    void readButton();
};

// =============================================
// ADDRESSABLE LED CLASS
// =============================================

class AddressableLEDs {
private:
    Adafruit_NeoPixel* ledStrip;
    uint8_t pin;
    uint16_t ledCount;
    unsigned long lastUpdate;
    
public:
    AddressableLEDs(uint8_t ledPin, uint16_t count);
    ~AddressableLEDs();
    
    void begin();
    void update();
    
    // Set LED colors based on institution
    void setInstitutionLED(int institution, uint32_t color);
    void turnOffAll();
    void blinkLED(int ledIndex, int times = 3, int delayMs = 200);
    void setAllLEDs(uint32_t color);
    
    // Animation effects
    void showRainbow();
    void showPulse(uint32_t color);
    void showBlink(uint32_t color, int times = 3);
    
    // Helper function for creating colors
    static uint32_t createColor(uint8_t r, uint8_t g, uint8_t b);
    
private:
    void updateAnimation();
};

// =============================================
// INPUT HANDLER CLASS
// =============================================

class InputHandler {
private:
    KeypadButton* button1;  // Institution 1
    KeypadButton* button2;  // Institution 2
    KeypadButton* button3;  // Institution 3
    KeypadButton* button4;  // Additional function
    AddressableLEDs* ledStrip;
    
    int currentInstitution;
    int lastInstitution;
    unsigned long lastCheckTime;

public:
    InputHandler();
    ~InputHandler();
    
    void begin();
    void update();
    
    // Get current institution from keypad
    int getCurrentInstitution();  // Returns 1, 2, or 3
    
    // Check if institution has changed
    bool hasInstitutionChanged();
    
    // Get institution name
    String getInstitutionName();
    
    // LED control
    void setActiveInstitution(int institution);
    void blinkInstitutionLEDs();
    
    // Additional button handlers
    bool isButton4Pressed();

private:
    void initializeKeypad();
    void initializeLEDs();
    void updateLEDs();
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
