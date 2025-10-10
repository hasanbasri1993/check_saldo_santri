#ifndef BUZZER_FEEDBACK_H
#define BUZZER_FEEDBACK_H

#include <Arduino.h>
#include "config.h"

// =============================================
// BUZZER FEEDBACK CLASS
// =============================================

class BuzzerFeedback {
private:
    uint8_t buzzerPin;
    unsigned long lastToneTime;
    bool isTonePlaying;

    // Private helper methods
    void playTone(uint16_t frequency, uint32_t duration);
    void playToneAsync(uint16_t frequency);
    void stopTone();
    void playMelody(const uint16_t* frequencies, const uint32_t* durations, uint8_t length);

public:
    // Constructor
    BuzzerFeedback(uint8_t pin = BUZZER_PIN);

    // Initialization
    void begin();

    // Feedback pattern methods (non-blocking)
    void playClick();           // Single short beep for button press
    void playSuccess();         // Rising melody for successful operation
    void playError();           // Double beep for errors
    void playWarning();         // Long descending tone for warnings
    void playProcessingPulse(); // Slow pulse for processing indication

    // Processing feedback (for continuous indication)
    void startProcessingPulse();
    void stopProcessingPulse();

    // Update method (call in main loop)
    void update();

    // Immediate feedback (blocking - use sparingly)
    void playBlockingBeep(uint16_t frequency = BEEP_FREQ, uint32_t duration = BEEP_DURATION);
    void playBlockingPattern(uint8_t pattern);
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern BuzzerFeedback buzzer;

// =============================================
// PATTERN DEFINITIONS
// =============================================

// Success melody - ascending notes
const uint16_t successFrequencies[] = {800, 1000, 1200, 1500};
const uint32_t successDurations[] = {100, 100, 100, 200};

// Error pattern - double beep
const uint16_t errorFrequencies[] = {ERROR_FREQ, ERROR_FREQ};
const uint32_t errorDurations[] = {BEEP_DURATION, BEEP_DURATION};

// Warning pattern - long descending tone
const uint16_t warningFrequencies[] = {ERROR_FREQ};
const uint32_t warningDurations[] = {LONG_BEEP_DURATION};

// Processing pulse - alternating frequencies
const uint16_t processingFrequencies[] = {600, 800};
const uint32_t processingDurations[] = {PULSE_DURATION, PULSE_DURATION};

#endif // BUZZER_FEEDBACK_H
