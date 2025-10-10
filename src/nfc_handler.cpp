#include "nfc_handler.h"
#include "mybase64.h"
#include <ArduinoJson.h>

// =============================================
// CLASS IMPLEMENTATION
// =============================================


#define BLOCK_SIZE 16
#define LONG_TLV_SIZE 4
#define SHORT_TLV_SIZE 2


NFCHandler::NFCHandler(uint8_t ssPin, uint8_t clkPin, uint8_t misoPin, uint8_t mosiPin) :
    isInitialized(false), lastCardCheck(0), cardTimeout(0) {

    // Initialize PN532 with I2C
    nfc = new Adafruit_PN532(I2C_SDA_PIN, I2C_SCL_PIN);
}

bool NFCHandler::begin() {
    Serial.println("Initializing NFC Reader...");

    initPN532();

    if (!isInitialized) {
        lastError = "Failed to initialize PN532";
        return false;
    }

    Serial.println("NFC Reader initialized successfully");
    return true;
}

void NFCHandler::initPN532() {
    //Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (nfc->begin()) {
        Serial.println("PN532 found");

        uint32_t versiondata = nfc->getFirmwareVersion();
        if (versiondata) {
            Serial.print("Found chip PN5");
            Serial.println((versiondata >> 24) & 0xFF, HEX);
            Serial.print("Firmware ver. ");
            Serial.print((versiondata >> 16) & 0xFF, DEC);
            Serial.print('.');
            Serial.println((versiondata >> 8) & 0xFF, DEC);

            // Configure board to read RFID tags
            nfc->SAMConfig();

            isInitialized = true;
            lastError = "";
        } else {
            lastError = "Failed to get PN532 firmware version";
            Serial.println("Didn't find PN53x board");
        }
    } else {
        lastError = "Failed to find PN532 board";
        Serial.println("Didn't find PN53x board");
    }
}

bool NFCHandler::isCardPresent() {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    success = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        return true;
    }

    return false;
}

String NFCHandler::getCardUID() {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    success = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        return bytesToHexString(uid, uidLength);
    }

    return "";  // No card present
}

bool NFCHandler::readSantriData(String& nama, String& induk) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    uint8_t data[320];  // Buffer for NDEF message

    // First, get the card UID
    success = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (!success) {
        lastError = "No card present";
        return false;
    }

    // Try to read NDEF data
    success = nfc->ntag2xx_ReadPage(4, data);  // Start reading from page 4

    if (success) {
        // Look for NDEF TLV (Type Length Value) structure
        // TLV format: T (1 byte), L (1 or 3 bytes), V (L bytes)
        uint16_t ndefStart = 0;
        bool foundNDEF = false;

        for (uint16_t i = 0; i < sizeof(data) - 2; i++) {
            if (data[i] == 0x03) {  // NDEF Message TLV
                ndefStart = i + 2;  // Skip T and L bytes
                foundNDEF = true;
                break;
            }
        }

        if (foundNDEF) {
            // Extract NDEF message length (simplified)
            uint16_t ndefLength = data[ndefStart - 1];  // L byte
            uint8_t* ndefMessage = &data[ndefStart];

            // Decode NDEF message
            return decodeNDEFMessage(ndefMessage, ndefLength, nama, induk);
        } else {
            lastError = "No NDEF message found on card";
            return false;
        }
    } else {
        lastError = "Failed to read NDEF data from card";
        return false;
    }
}

String NFCHandler::bytesToHexString(uint8_t* data, uint8_t length) {
    String hexString = "";

    for (uint8_t i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            hexString += "0";
        }
        hexString += String(data[i], HEX);
    }

    return hexString;
}

bool NFCHandler::decodeNDEFMessage(uint8_t* message, uint16_t messageLength, String& nama, String& induk) {
    // Simple NDEF text record decoder
    // This is a simplified implementation - in production, use proper NDEF library

    if (messageLength < 10) {
        lastError = "NDEF message too short";
        return false;
    }

    // Check for NDEF record header
    if (message[0] != 0xD1) {  // Text record header (simplified check)
        lastError = "Not a valid NDEF text record";
        return false;
    }

    // Extract JSON data from the record payload
    uint8_t payloadLength = message[1];  // Simplified - actual parsing is more complex

    if (payloadLength > messageLength - 2) {
        lastError = "Invalid payload length";
        return false;
    }

    // Convert payload to string (assuming UTF-8)
    String jsonString = "";
    for (uint8_t i = 2; i < 2 + payloadLength && i < messageLength; i++) {
        jsonString += (char)message[i];
    }

    // Parse JSON
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        lastError = "Failed to parse JSON: " + String(error.c_str());
        return false;
    }

    // Extract nama and induk
    if (doc.containsKey("nama") && doc.containsKey("induk")) {
        nama = doc["nama"].as<String>();
        induk = doc["induk"].as<String>();
        return true;
    } else {
        lastError = "JSON missing nama or induk fields";
        return false;
    }
}

void NFCHandler::printCardInfo(uint8_t* uid, uint8_t uidLength) {
    Serial.println("=== Card Information ===");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i] < 0x10 ? " 0" : " ");
        Serial.print(uid[i], HEX);
    }
    Serial.println();

    String uidString = bytesToHexString(uid, uidLength);
    Serial.print("UID String: ");
    Serial.println(uidString);
}

bool NFCHandler::authenticateCard() {
    // For Mifare Classic cards, authentication might be needed
    // This is a placeholder - implement based on specific card requirements
    return true;
}

void NFCHandler::update() {
    // Update method for continuous operations if needed
    // Currently, NFC operations are event-driven
}

String NFCHandler::getLastError() const {
    return lastError;
}

// =============================================
// UTILITY FUNCTIONS
// =============================================

String waitForCardUID(uint32_t timeoutMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < timeoutMs) {
        String uid = nfcHandler.getCardUID();
        if (uid.length() > 0) {
            return uid;
        }
        delay(100);  // Small delay to prevent busy waiting
    }

    return "";  // Timeout
}

bool isCardStillPresent() {
    return nfcHandler.isCardPresent();
}


String CPrintHexChar(const byte *data, const long numBytes)
{
  int32_t szPos;
  String ll;
  char decoded[512];
  JsonDocument doc;

  for (szPos = 10; szPos < numBytes; szPos++)
  {
    if (data[szPos] <= 0x1F)
      Serial.print("");
    else
    {
      ll += (char)data[szPos];
    }
  }
  b64_decode(decoded, (char *)ll.c_str(), ll.length());
  DeserializationError error = deserializeJson(doc, decoded);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return "error";
  }
  // const char *id = doc["id"];                       // "5744"
  const char *induk = doc["induk"]; // "196600"
  // const char *nik = doc["nik"];                     // "3201266712060003"
  const char *nama = doc["nama"]; // "Inggrit Destiana Nugraeni"
  // const char *alamat = doc["alamat"];               // "Jln Megamendung Rt.04/04 Blok C No 20 CIPAYUNG (CIPAYUNG DATAR) ...
  // const char *kabupaten = doc["kabupaten"];         // "BOGOR"
  // const char *tempat_lahir = doc["tempat_lahir"];   // "BOGOR"
  // const char *tanggal_lahir = doc["tanggal_lahir"]; // "2006-12-27"
  // const char *nisn = doc["nisn"];                   // "0063760230"
  // const char *foto = doc["foto"];                   // "5c78eca25cf0836cc15623b4c18d9336.JPG"
  String result = String(induk) + "-" + String(nama);
  return result;
}

int getNdefStartIndex(byte *data)
{
  for (int i = 0; i < BLOCK_SIZE; i++)
  {
    if (data[i] == 0x0)
    {
    }
    else if (data[i] == 0x3)
    {
      return i;
    }
    else
    {
      Serial.print("Unknown TLV ");
      return -2;
    }
  }
  return -1;
}

int getBufferSize(int messageLength)
{
  int bufferSize = messageLength;
  if (messageLength < 0xFF)
  {
    bufferSize += SHORT_TLV_SIZE + 1;
  }
  else
  {
    bufferSize += LONG_TLV_SIZE + 1;
  }
  if (bufferSize % BLOCK_SIZE != 0)
  {
    bufferSize = ((bufferSize / BLOCK_SIZE) + 1) * BLOCK_SIZE;
  }
  return bufferSize;
}

bool decodeTlv(byte *data, int &messageLength, int &messageStartIndex)
{
  int i = getNdefStartIndex(data);
  if (i < 0 || data[i] != 0x3)
  {
    Serial.println(F("Error. Can't decode message length."));
    return false;
  }
  else
  {
    if (data[i + 1] == 0xFF)
    {
      messageLength = ((0xFF & data[i + 2]) << 8) | (0xFF & data[i + 3]);
      messageStartIndex = i + LONG_TLV_SIZE;
    }
    else
    {
      messageLength = data[i + 1];
      messageStartIndex = i + SHORT_TLV_SIZE;
    }
  }
  return true;
}

String getCardTypeString(uint8_t cardType) {
    switch (cardType) {
        case 0x00:
            return "Mifare Ultralight";
        case 0x01:
            return "Mifare Classic 1K";
        case 0x02:
            return "Mifare Classic 4K";
        case 0x03:
            return "Mifare DESFire";
        default:
            return "Unknown";
    }
}

// =============================================
// GLOBAL INSTANCE
// =============================================

NFCHandler nfcHandler;
