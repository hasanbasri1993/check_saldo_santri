#include "simple_led.h"

// =============================================
// GLOBAL LED INSTANCE
// =============================================

SimpleLED simpleLED;

// =============================================
// SIMPLE LED CLASS IMPLEMENTATION
// =============================================

SimpleLED::SimpleLED() : 
    currentState(LED_OFF),
    previousState(LED_OFF),
    stateStartTime(0),
    lastUpdateTime(0),
    isAnimating(false),
    breathingBrightness(0),
    breathingDirection(true),
    rainbowHue(0),
    blinkState(false),
    ledOn(false),
    currentBrightness(255),
    pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {
}

SimpleLED::~SimpleLED() {
    shutdown();
}

bool SimpleLED::init() {
    Serial.println("Initializing Built-in RGB LED (WS2812B) for WeAct Studio ESP32-S3...");
    
    // Initialize NeoPixel
    pixels.begin();
    pixels.clear();
    pixels.show();
    
    currentState = LED_BOOTING;
    stateStartTime = millis();
    lastUpdateTime = millis();
    isAnimating = true;
    
    Serial.println("Built-in RGB LED (WS2812B) initialized successfully");
    Serial.printf("LED Pin: %d (Built-in RGB LED)\n", LED_PIN);
    
    // Test LED with different colors (non-blocking)
    Serial.println("Testing built-in RGB LED...");
    
    // Quick test sequence without blocking delays
    Serial.println("Testing RED...");
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    
    Serial.println("Testing GREEN...");
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
    
    Serial.println("Testing BLUE...");
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    
    Serial.println("Testing WHITE...");
    pixels.setPixelColor(0, pixels.Color(255, 255, 255));
    pixels.show();
    
    // Turn off
    pixels.clear();
    pixels.show();
    
    Serial.println("Built-in RGB LED test completed!");
    
    return true;
}

void SimpleLED::shutdown() {
    pixels.clear();
    pixels.show();
    currentState = LED_OFF;
    isAnimating = false;
    Serial.println("Built-in RGB LED shutdown");
}

void SimpleLED::setState(LEDState newState) {
    if (newState != currentState) {
        previousState = currentState;
        currentState = newState;
        stateStartTime = millis();
        lastUpdateTime = millis();
        isAnimating = true;
        
        Serial.printf("LED State changed: %d -> %d\n", previousState, currentState);
        
        // Reset animation variables
        breathingBrightness = 0;
        breathingDirection = true;
        rainbowHue = 0;
        blinkState = false;
    }
}

LEDState SimpleLED::getCurrentState() const {
    return currentState;
}

void SimpleLED::update() {
    unsigned long currentTime = millis();
    
    // Non-blocking update - only process if enough time has passed
    if (currentTime - lastUpdateTime < 50) { // 50ms update interval
        return;
    }
    
    lastUpdateTime = currentTime;
    
    if (!isAnimating) {
        return;
    }
    
    // Handle different states
    switch (currentState) {
        case LED_BOOTING:
            showBootingPattern();
            break;
            
        case LED_WIFI_CONNECTING:
            showWiFiConnectingPattern();
            break;
            
        case LED_WIFI_CONNECTED:
            showWiFiConnectedPattern();
            break;
            
        case LED_WIFI_ERROR:
            showWiFiErrorPattern();
            break;
            
        case LED_OTA_PROGRESS:
            showOTAProgressPattern();
            break;
            
        case LED_CARD_READING:
            showCardReadingPattern();
            break;
            
        case LED_CARD_VALID:
            showCardValidPattern();
            break;
            
        case LED_CARD_INVALID:
            showCardInvalidPattern();
            break;
            
        case LED_SERVER_ERROR:
            showServerErrorPattern();
            break;
            
        case LED_OFF:
        default:
            turnOff();
            break;
    }
}

void SimpleLED::showBootingPattern() {
    // Breathing effect for 5 seconds with white/blue color
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed >= LED_BREATHING_DURATION) {
        setState(LED_OFF);
        return;
    }
    
    updateBreathing();
    // Use blue color for booting
    setLEDColor(0, 0, breathingBrightness);
}

void SimpleLED::showWiFiConnectingPattern() {
    // Blink with 500ms interval - blue color
    updateBlink();
    if (blinkState) {
        setLEDColor(0, 0, 255); // Blue
    } else {
        setLED(false);
    }
}

void SimpleLED::showWiFiConnectedPattern() {
    // Solid green for 3 seconds
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed >= LED_WIFI_CONNECTED_DURATION) {
        setState(LED_OFF);
        return;
    }
    
    setLEDColor(0, 255, 0); // Green
}

void SimpleLED::showWiFiErrorPattern() {
    // Fast blink with 250ms interval - red color
    updateBlink();
    if (blinkState) {
        setLEDColor(255, 0, 0); // Red
    } else {
        setLED(false);
    }
}

void SimpleLED::showOTAProgressPattern() {
    // Rainbow effect with color cycling
    updateRainbow();
    
    // Convert hue to RGB for rainbow effect
    uint8_t r, g, b;
    if (rainbowHue < 85) {
        r = 255 - rainbowHue * 3;
        g = rainbowHue * 3;
        b = 0;
    } else if (rainbowHue < 170) {
        rainbowHue -= 85;
        r = 0;
        g = 255 - rainbowHue * 3;
        b = rainbowHue * 3;
    } else {
        rainbowHue -= 170;
        r = rainbowHue * 3;
        g = 0;
        b = 255 - rainbowHue * 3;
    }
    
    setLEDColor(r, g, b);
}

void SimpleLED::showCardReadingPattern() {
    // Solid yellow for card reading
    setLEDColor(255, 255, 0); // Yellow
}

void SimpleLED::showCardValidPattern() {
    // Solid green for valid card
    setLEDColor(0, 255, 0); // Green
}

void SimpleLED::showCardInvalidPattern() {
    // Solid red for invalid card
    setLEDColor(255, 0, 0); // Red
}

void SimpleLED::showServerErrorPattern() {
    // Fast blink red for server error
    updateBlink();
    if (blinkState) {
        setLEDColor(255, 0, 0); // Red
    } else {
        setLED(false);
    }
}

void SimpleLED::turnOff() {
    setLED(false);
    isAnimating = false;
}

void SimpleLED::setLED(bool on, uint8_t brightness) {
    ledOn = on;
    currentBrightness = brightness;
    
    if (on) {
        // Set brightness and show current color
        pixels.setBrightness(brightness);
        pixels.show();
    } else {
        pixels.clear();
        pixels.show();
    }
}

void SimpleLED::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    // Set RGB color for WS2812B LED
    if (r == 0 && g == 0 && b == 0) {
        pixels.clear();
        pixels.show();
        ledOn = false;
        return;
    }
    
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
    ledOn = true;
}

void SimpleLED::updateBreathing() {
    // Breathing effect: fade in and out
    if (breathingDirection) {
        breathingBrightness += 5;
        if (breathingBrightness >= 255) {
            breathingBrightness = 255;
            breathingDirection = false;
        }
    } else {
        breathingBrightness -= 5;
        if (breathingBrightness <= 0) {
            breathingBrightness = 0;
            breathingDirection = true;
        }
    }
}

void SimpleLED::updateRainbow() {
    // Rainbow cycle effect
    rainbowHue += 2;
    if (rainbowHue >= 255) {
        rainbowHue = 0;
    }
}

void SimpleLED::updateBlink() {
    // Blink effect based on current state
    unsigned long interval = LED_BLINK_INTERVAL_CONNECTING;
    
    if (currentState == LED_WIFI_ERROR || currentState == LED_SERVER_ERROR) {
        interval = LED_BLINK_INTERVAL_ERROR;
    }
    
    if (millis() - stateStartTime >= interval) {
        blinkState = !blinkState;
        stateStartTime = millis();
    }
}

void SimpleLED::printState() const {
    Serial.printf("LED State: %d, Animating: %s, Time: %lu\n", 
                  currentState, isAnimating ? "Yes" : "No", millis() - stateStartTime);
}

// =============================================
// CONVENIENCE FUNCTIONS
// =============================================

void setLEDState(LEDState state) {
    simpleLED.setState(state);
}

void ledLoop() {
    simpleLED.update();
}
