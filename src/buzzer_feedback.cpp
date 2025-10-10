#include "buzzer_feedback.h"

// =============================================
// CLASS IMPLEMENTATION
// =============================================

BuzzerFeedback::BuzzerFeedback(uint8_t pin) : buzzerPin(pin), lastToneTime(0), isTonePlaying(false) {}

void BuzzerFeedback::begin() {
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    isTonePlaying = false;
}

void BuzzerFeedback::playTone(uint16_t frequency, uint32_t duration) {
    tone(buzzerPin, frequency, duration);
    delay(duration);
    noTone(buzzerPin);
}

void BuzzerFeedback::playToneAsync(uint16_t frequency) {
    if (!isTonePlaying) {
        tone(buzzerPin, frequency);
        isTonePlaying = true;
        lastToneTime = millis();
    }
}

void BuzzerFeedback::stopTone() {
    if (isTonePlaying) {
        noTone(buzzerPin);
        isTonePlaying = false;
    }
}

void BuzzerFeedback::playMelody(const uint16_t* frequencies, const uint32_t* durations, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        playTone(frequencies[i], durations[i]);
        delay(50); // Small pause between notes
    }
}

void BuzzerFeedback::playClick() {
    playTone(BEEP_FREQ, BEEP_DURATION);
}

void BuzzerFeedback::playSuccess() {
    playMelody(successFrequencies, successDurations, sizeof(successFrequencies) / sizeof(successFrequencies[0]));
}

void BuzzerFeedback::playError() {
    playMelody(errorFrequencies, errorDurations, sizeof(errorFrequencies) / sizeof(errorFrequencies[0]));
    delay(100); // Pause between double beeps
    playMelody(errorFrequencies, errorDurations, sizeof(errorFrequencies) / sizeof(errorFrequencies[0]));
}

void BuzzerFeedback::playWarning() {
    playMelody(warningFrequencies, warningDurations, sizeof(warningFrequencies) / sizeof(warningFrequencies[0]));
}

void BuzzerFeedback::playProcessingPulse() {
    static uint8_t pulseState = 0;

    if (millis() - lastToneTime >= PULSE_DURATION) {
        if (pulseState == 0) {
            playTone(processingFrequencies[0], processingDurations[0]);
            pulseState = 1;
        } else {
            playTone(processingFrequencies[1], processingDurations[1]);
            pulseState = 0;
        }
        lastToneTime = millis();
    }
}

void BuzzerFeedback::startProcessingPulse() {
    // This method can be used to initialize processing pulse state if needed
    lastToneTime = millis();
}

void BuzzerFeedback::stopProcessingPulse() {
    stopTone();
}

void BuzzerFeedback::update() {
    // This method is called in the main loop for continuous feedback patterns
    // Currently, no continuous patterns need updating beyond timing
}

void BuzzerFeedback::playBlockingBeep(uint16_t frequency, uint32_t duration) {
    playTone(frequency, duration);
}

void BuzzerFeedback::playBlockingPattern(uint8_t pattern) {
    switch (pattern) {
        case PATTERN_CLICK:
            playClick();
            break;
        case PATTERN_SUCCESS:
            playSuccess();
            break;
        case PATTERN_ERROR:
            playError();
            break;
        case PATTERN_WARNING:
            playWarning();
            break;
        default:
            break;
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

BuzzerFeedback buzzer;
