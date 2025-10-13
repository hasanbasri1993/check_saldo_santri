#include "input_handler.h"

// =============================================
// TOGGLE SWITCH CLASS IMPLEMENTATION
// =============================================

ToggleSwitch::ToggleSwitch(uint8_t switchPinA, uint8_t switchPinB, uint16_t debounceMs) :
    pinA(switchPinA), pinB(switchPinB), lastPosition(2), currentPosition(2),
    lastDebounceTime(0), debounceDelay(debounceMs) {}

void ToggleSwitch::begin() {
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    
    // Read initial position
    readPosition();
    lastPosition = currentPosition;
    
    Serial.printf("Toggle Switch initialized - Pin A: %d, Pin B: %d, Initial position: %d\n", 
                 pinA, pinB, currentPosition);
}

void ToggleSwitch::update() {
    readPosition();
}

void ToggleSwitch::readPosition() {
    bool stateA = digitalRead(pinA);
    bool stateB = digitalRead(pinB);
    
    int newPosition;
    
    if (stateA == LOW) {
        // Pin A is LOW - Position 1 (Institution 1)
        newPosition = 1;
    } else if (stateB == LOW) {
        // Pin B is LOW - Position 3 (Institution 3)
        newPosition = 3;
    } else {
        // Both pins HIGH - Position 2 (Institution 2) - OFF position
        newPosition = 2;
    }
    
    if (newPosition != currentPosition) {
        unsigned long currentTime = millis();
        if (currentTime - lastDebounceTime > debounceDelay) {
            Serial.printf("Toggle Switch: Position changed from %d to %d\n", currentPosition, newPosition);
            lastPosition = currentPosition;
            currentPosition = newPosition;
            lastDebounceTime = currentTime;
        }
    }
}

int ToggleSwitch::getCurrentPosition() {
    return currentPosition;
}

bool ToggleSwitch::hasPositionChanged() {
    if (currentPosition != lastPosition) {
        lastPosition = currentPosition;
        return true;
    }
    return false;
}

String ToggleSwitch::getPositionName() {
    switch (currentPosition) {
        case 1: return "INST_1";
        case 2: return "INST_2";
        case 3: return "INST_3";
        default: return "UNKNOWN";
    }
}

// =============================================
// INSTITUTION LED INDICATOR CLASS IMPLEMENTATION
// =============================================

InstitutionLEDs::InstitutionLEDs(uint8_t pin1, uint8_t pin2, uint8_t pin3) :
    led1Pin(pin1), led2Pin(pin2), led3Pin(pin3), currentActiveLED(0) {}

void InstitutionLEDs::begin() {
    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);
    pinMode(led3Pin, OUTPUT);
    
    turnOffAll();
    
    Serial.printf("Institution LEDs initialized - Pin 1: %d, Pin 2: %d, Pin 3: %d\n", 
                 led1Pin, led2Pin, led3Pin);
}

void InstitutionLEDs::setActiveInstitution(int institution) {
    currentActiveLED = institution;
    updateLEDs();
}

void InstitutionLEDs::turnOffAll() {
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    currentActiveLED = 0;
}

void InstitutionLEDs::blinkAll(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(led1Pin, HIGH);
        digitalWrite(led2Pin, HIGH);
        digitalWrite(led3Pin, HIGH);
        delay(delayMs);
        
        digitalWrite(led1Pin, LOW);
        digitalWrite(led2Pin, LOW);
        digitalWrite(led3Pin, LOW);
        delay(delayMs);
    }
}

void InstitutionLEDs::updateLEDs() {
    // Turn off all LEDs first
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    
    // Turn on the active LED
    switch (currentActiveLED) {
        case 1:
            digitalWrite(led1Pin, HIGH);
            break;
        case 2:
            digitalWrite(led2Pin, HIGH);
            break;
        case 3:
            digitalWrite(led3Pin, HIGH);
            break;
    }
}

// =============================================
// INPUT HANDLER CLASS IMPLEMENTATION
// =============================================

InputHandler::InputHandler() : toggleSwitch(nullptr), institutionLEDs(nullptr), lastCheckTime(0) {}

void InputHandler::begin() {
    initializeToggleSwitch();
    initializeInstitutionLEDs();
}

void InputHandler::initializeToggleSwitch() {
    Serial.println("Initializing Toggle Switch...");
    toggleSwitch = new ToggleSwitch(SWITCH_PIN_A, SWITCH_PIN_B);
    toggleSwitch->begin();
    Serial.println("Toggle Switch initialized successfully!");
}

void InputHandler::initializeInstitutionLEDs() {
    Serial.println("Initializing Institution LEDs...");
    institutionLEDs = new InstitutionLEDs(LED_INST_1_PIN, LED_INST_2_PIN, LED_INST_3_PIN);
    institutionLEDs->begin();
    
    // Set initial LED based on switch position
    int currentInstitution = getCurrentInstitution();
    setActiveInstitution(currentInstitution);
    
    Serial.println("Institution LEDs initialized successfully!");
}

void InputHandler::update() {
    if (millis() - lastCheckTime >= 50) {  // Update every 50ms
        if (toggleSwitch) {
            toggleSwitch->update();
        }
        lastCheckTime = millis();
    }
}

int InputHandler::getCurrentInstitution() {
    if (toggleSwitch) {
        return toggleSwitch->getCurrentPosition();
    }
    return 2;  // Default to Institution 2 (OFF position)
}

bool InputHandler::hasInstitutionChanged() {
    if (toggleSwitch) {
        return toggleSwitch->hasPositionChanged();
    }
    return false;
}

String InputHandler::getInstitutionName() {
    if (toggleSwitch) {
        return toggleSwitch->getPositionName();
    }
    return "INST_2";
}

void InputHandler::setActiveInstitution(int institution) {
    if (institutionLEDs) {
        institutionLEDs->setActiveInstitution(institution);
    }
}

void InputHandler::blinkInstitutionLEDs() {
    if (institutionLEDs) {
        institutionLEDs->blinkAll();
    }
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

String getInstitutionName(int institution) {
    switch (institution) {
        case 1: return "INSTITUTION_1";
        case 2: return "INSTITUTION_2";
        case 3: return "INSTITUTION_3";
        default: return "UNKNOWN";
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

InputHandler inputHandler;
