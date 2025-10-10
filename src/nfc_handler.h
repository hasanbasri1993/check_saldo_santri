#ifndef NFC_HANDLER_H
#define NFC_HANDLER_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "config.h"

// =============================================
// NFC HANDLER CLASS
// =============================================

class NFCHandler {
private:
    Adafruit_PN532* nfc;
    bool isInitialized;
    unsigned long lastCardCheck;
    uint8_t cardTimeout;

    // Helper methods
    bool waitForCard(uint32_t timeoutMs);
    String bytesToHexString(uint8_t* data, uint8_t length);
    bool decodeNDEFMessage(uint8_t* message, uint16_t messageLength, String& nama, String& induk);

public:
    NFCHandler(uint8_t ssPin = SS, uint8_t clkPin = -1, uint8_t misoPin = -1, uint8_t mosiPin = -1);

    // Initialization
    bool begin();
    void initPN532();

    // Card detection and reading
    bool isCardPresent();
    String getCardUID();  // Returns UID as hex string

    // NDEF reading for santri data
    bool readSantriData(String& nama, String& induk);

    // Utility methods
    void printCardInfo(uint8_t* uid, uint8_t uidLength);
    bool authenticateCard();  // For Mifare Classic cards if needed

    // Update method (call in main loop)
    void update();

    // Status
    bool isReady() const { return isInitialized; }
    String getLastError() const;

private:
    String lastError;
};

// =============================================
// GLOBAL INSTANCE
// =============================================

extern NFCHandler nfcHandler;

// =============================================
// UTILITY FUNCTIONS
// =============================================

// Non-blocking card detection with timeout
String waitForCardUID(uint32_t timeoutMs = CARD_READ_TIMEOUT);

// Check if card is still present
bool isCardStillPresent();

// Get card type as string
String getCardTypeString(uint8_t cardType);

#endif // NFC_HANDLER_H
