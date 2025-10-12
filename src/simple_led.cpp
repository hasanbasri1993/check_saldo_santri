#include "simple_led.h"

// =============================================
// GLOBAL LED INSTANCE
// =============================================

SimpleLED simpleLED;

// =============================================
// SIMPLE LED CLASS IMPLEMENTATION
// =============================================

SimpleLED::SimpleLED() : currentState(LED_OFF),
                         previousState(LED_OFF),
                         stateStartTime(0),
                         lastUpdateTime(0),
                         isAnimating(false),
                         breathingBrightness(0),
                         breathingDirection(true),
                         rainbowHue(0),
                         blinkState(false),
                         ledOn(false),
                         currentBrightness(100), // Default brightness like in neo.md
                         pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800)
{ // Use GPIO 48 and 1 pixel like in neo.md
}

SimpleLED::~SimpleLED()
{
    shutdown();
}

bool SimpleLED::init()
{
    Serial.println("Initializing Built-in RGB LED (WS2812B) for WeAct Studio ESP32-S3...");

    // Initialize NeoPixel exactly like in neo.md - simple and direct
    pixels.begin();
    pixels.setBrightness(100); // Set brightness like in neo.md

    currentState = LED_BOOTING;
    stateStartTime = millis();
    lastUpdateTime = millis();
    isAnimating = true;

    Serial.println("Built-in RGB LED (WS2812B) initialized successfully");
    Serial.printf("LED Pin: %d (Built-in RGB LED)\n", LED_PIN);

    Serial.println("Built-in RGB LED test completed!");

    return true;
}

void SimpleLED::shutdown()
{
    pixels.clear();
    pixels.show();
    currentState = LED_OFF;
    isAnimating = false;
    Serial.println("Built-in RGB LED shutdown");
}

void SimpleLED::setState(LEDState newState)
{
    if (newState != currentState)
    {
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

LEDState SimpleLED::getCurrentState() const
{
    return currentState;
}

void SimpleLED::update()
{
    unsigned long currentTime = millis();

    // Non-blocking update - only process if enough time has passed
    if (currentTime - lastUpdateTime < 50)
    { // 50ms update interval
        return;
    }

    lastUpdateTime = currentTime;

    if (!isAnimating)
    {
        return;
    }

    // Handle different states
    switch (currentState)
    {
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

void SimpleLED::showBootingPattern()
{
    // Breathing effect for 5 seconds with white/blue color
    unsigned long elapsed = millis() - stateStartTime;

    if (elapsed >= LED_BREATHING_DURATION)
    {
        setState(LED_OFF);
        return;
    }

    updateBreathing();
    // Use blue color for booting with HSV
    setLEDColorHSV(43690, 255, breathingBrightness); // Blue with breathing brightness
}

void SimpleLED::showWiFiConnectingPattern()
{
    // Blink with 500ms interval - blue color
    updateBlink();
    if (blinkState)
    {
        setLEDColorHSV(43690, 255, 255); // Blue
    }
    else
    {
        setLED(false);
    }
}

void SimpleLED::showWiFiConnectedPattern()
{
    // Solid green for 3 seconds using HSV
    unsigned long elapsed = millis() - stateStartTime;

    if (elapsed >= LED_WIFI_CONNECTED_DURATION)
    {
        setState(LED_OFF);
        return;
    }

    setLEDColorHSV(21845, 255, 255); // Green
}

void SimpleLED::showWiFiErrorPattern()
{
    // Fast blink with 250ms interval - red color using HSV
    updateBlink();
    if (blinkState)
    {
        setLEDColorHSV(0, 255, 255); // Red
    }
    else
    {
        setLED(false);
    }
}

void SimpleLED::showOTAProgressPattern()
{
    // Rainbow effect with color cycling using HSV like in neo.md
    updateRainbow();

    // Use HSV directly for rainbow effect
    setLEDColorHSV(rainbowHue, 255, 255); // Use 16-bit hue directly
}

void SimpleLED::showCardReadingPattern()
{
    // Solid yellow for card reading using HSV
    setLEDColorHSV(10922, 255, 255); // Yellow (hue = 60 degrees)
}

void SimpleLED::showCardValidPattern()
{
    // Solid green for valid card using HSV
    setLEDColorHSV(21845, 255, 255); // Green (hue = 120 degrees)
}

void SimpleLED::showCardInvalidPattern()
{
    // Solid red for invalid card using HSV
    setLEDColorHSV(0, 255, 255); // Red (hue = 0 degrees)
}

void SimpleLED::showServerErrorPattern()
{
    // Fast blink red for server error using HSV
    updateBlink();
    if (blinkState)
    {
        setLEDColorHSV(0, 255, 255); // Red
    }
    else
    {
        setLED(false);
    }
}

void SimpleLED::turnOff()
{
    setLED(false);
    isAnimating = false;
}

void SimpleLED::setLED(bool on, uint8_t brightness)
{
    ledOn = on;
    currentBrightness = brightness;

    if (on)
    {
        // Set brightness and show current color
        pixels.setBrightness(brightness);
        pixels.show();
    }
    else
    {
        pixels.clear();
        pixels.show();
    }
}

void SimpleLED::setLEDColor(uint8_t r, uint8_t g, uint8_t b)
{
    // Set RGB color for WS2812B LED
    if (r == 0 && g == 0 && b == 0)
    {
        pixels.clear();
        pixels.show();
        ledOn = false;
        return;
    }

    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
    ledOn = true;
}

// New method using HSV like in neo.md
void SimpleLED::setLEDColorHSV(uint16_t hue, uint8_t saturation, uint8_t value)
{
    uint32_t color = pixels.gamma32(pixels.ColorHSV(hue, saturation, value));
    pixels.setPixelColor(0, color);
    pixels.show();
    ledOn = true;
}

void SimpleLED::updateBreathing()
{
    // Breathing effect: fade in and out
    if (breathingDirection)
    {
        breathingBrightness += 5;
        if (breathingBrightness >= 255)
        {
            breathingBrightness = 255;
            breathingDirection = false;
        }
    }
    else
    {
        breathingBrightness -= 5;
        if (breathingBrightness <= 0)
        {
            breathingBrightness = 0;
            breathingDirection = true;
        }
    }
}

void SimpleLED::updateRainbow()
{
    // Rainbow cycle effect like in neo.md
    rainbowHue += 256; // Smooth step like in neo.md
    if (rainbowHue >= 65535)
    { // Full 16-bit cycle
        rainbowHue = 0;
    }
}

void SimpleLED::updateBlink()
{
    // Blink effect based on current state
    unsigned long interval = LED_BLINK_INTERVAL_CONNECTING;

    if (currentState == LED_WIFI_ERROR || currentState == LED_SERVER_ERROR)
    {
        interval = LED_BLINK_INTERVAL_ERROR;
    }

    if (millis() - stateStartTime >= interval)
    {
        blinkState = !blinkState;
        stateStartTime = millis();
    }
}

void SimpleLED::printState() const
{
    Serial.printf("LED State: %d, Animating: %s, Time: %lu\n",
                  currentState, isAnimating ? "Yes" : "No", millis() - stateStartTime);
}

// =============================================
// CONVENIENCE FUNCTIONS
// =============================================

void setLEDState(LEDState state)
{
    simpleLED.setState(state);
}

void ledLoop()
{
    simpleLED.update();
}
