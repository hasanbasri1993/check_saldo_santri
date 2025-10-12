#ifndef SIMPLE_LED_H
#define SIMPLE_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// =============================================
// SIMPLE LED MANAGER CLASS
// =============================================

class SimpleLED {
private:
    LEDState currentState;
    LEDState previousState;
    unsigned long stateStartTime;
    unsigned long lastUpdateTime;
    bool isAnimating;
    
    // Animation variables
    uint8_t breathingBrightness;
    bool breathingDirection;
    uint16_t rainbowHue; // Changed to 16-bit for HSV like in neo.md
    bool blinkState;
    
    // LED control variables
    bool ledOn;
    uint8_t currentBrightness;
    Adafruit_NeoPixel pixels;
    
public:
    SimpleLED();
    ~SimpleLED();
    
    // Initialization
    bool init();
    void shutdown();
    
    // State management
    void setState(LEDState newState);
    LEDState getCurrentState() const;
    void update(); // Non-blocking update function
    
    // Pattern functions
    void showBootingPattern();
    void showWiFiConnectingPattern();
    void showWiFiConnectedPattern();
    void showWiFiErrorPattern();
    void showOTAProgressPattern();
    void showCardReadingPattern();
    void showCardValidPattern();
    void showCardInvalidPattern();
    void showServerErrorPattern();
    void turnOff();
    
    // LED control functions
    void setLED(bool on, uint8_t brightness = 255);
    void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
    void setLEDColorHSV(uint16_t hue, uint8_t saturation, uint8_t value);
    
    // Animation helpers
    void updateBreathing();
    void updateRainbow();
    void updateBlink();
    
    // Debug
    void printState() const;
};

// =============================================
// GLOBAL LED INSTANCE
// =============================================

extern SimpleLED simpleLED;

// =============================================
// CONVENIENCE FUNCTIONS
// =============================================

// Easy state setting functions
void setLEDState(LEDState state);
void ledLoop(); // Call this in main loop()

#endif // SIMPLE_LED_H
