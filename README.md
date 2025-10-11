# Santri Card Reader dengan OTA Update

Sistem pembaca kartu santri berbasis ESP32 dengan fitur Over-The-Air (OTA) update untuk kemudahan maintenance tanpa perlu koneksi USB.

## Fitur Utama

- **Pembacaan Kartu NFC**: Menggunakan PN532 untuk membaca kartu santri dengan format NDEF
- **WiFi Manager**: Konfigurasi WiFi melalui captive portal
- **OTA Update**: Update firmware melalui web interface tanpa USB
- **LCD Display**: Interface 16x2 untuk status dan informasi
- **Buzzer Feedback**: Notifikasi audio untuk berbagai state
- **Button Interface**: 3 tombol untuk interaksi pengguna
- **API Integration**: Komunikasi dengan server untuk validasi dan logging

## Hardware Requirements

- **ESP32-S3 DevKitC-1** (atau kompatibel)
- **PN532 NFC Reader** (I2C interface)
- **LCD 16x2 dengan I2C backpack**
- **3 Push Buttons**
- **Buzzer (aktif/pasif)**
- **Power supply 5V**

## Pin Configuration

| Component | GPIO Pin | Keterangan |
|-----------|----------|------------|
| I2C SDA | GPIO8 | Data line untuk LCD dan PN532 |
| I2C SCL | GPIO9 | Clock line untuk LCD dan PN532 |
| Button 1 | GPIO15 | Institution 1 |
| Button 2 | GPIO4 | Institution 2 |
| Button 3 | GPIO5 | Institution 3 |
| Buzzer | GPIO19 | Audio feedback |

## OTA (Over-The-Air) Update

### Setup OTA
1. OTA berjalan di background setelah WiFi tersambung (tidak mengganggu interface kartu)
2. Serial monitor akan menunjukkan URL OTA: `http://[device_ip]:8080/`

### Cara Update Firmware
1. **Buka browser** dan akses: `http://[device_ip]:8080/`
2. **Klik** tombol "Go to OTA Update"
3. **Login** dengan:
   - Username: `admin`
   - Password: `santri123`
4. **Upload file .bin** hasil build dari PlatformIO
5. **Tunggu progress** dan device akan reboot otomatis

### Web Interface Endpoints

| Endpoint | Fungsi | Authentication |
|----------|--------|----------------|
| `http://ip:8080/` | Dashboard device info | Tidak perlu |
| `http://ip:8080/upload` | Form upload firmware | Tidak perlu |
| `http://ip:8080/update` | OTA firmware upload | `admin:santri123` |
| `http://ip:8080/info` | JSON device info | Tidak perlu |

## API Integration

### Card Validation
```http
GET /api/v1/santri/validate?id_card={uid}&id_santri={santri_id}&id_device={mac_address}
```

### Activity Logging
```http
POST /api/v1/santri/visitor_santri/
Content-Type: application/x-www-form-urlencoded

memberID={santri_id}&counter=1&institution={1,2,3}
```

## Development Setup

### PlatformIO Configuration
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
framework = arduino
monitor_speed = 115200

lib_deps =
    tzapu/WiFiManager@^2.0.14
    adafruit/Adafruit PN532@^1.3.3
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
    bblanchon/ArduinoJson@^7.4.2
    me-no-dev/ESP Async WebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
```

### Build Flags
```ini
build_flags =
    -D ELEGANTOTA_USE_ASYNC_WEBSERVER=1
    -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000
    -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64
    -D ARDUINOJSON_USE_LONG_LONG=1
```

## Troubleshooting

### OTA Issues
1. **OTA tidak muncul**: Periksa WiFi connection dan port 8080
2. **Upload gagal**: Pastikan file .bin valid dan ukuran sesuai flash
3. **Authentication error**: Gunakan username `admin` dan password `santri123`

### I2C Issues
1. **Error I2C transaction**: Periksa koneksi hardware dan pin configuration
2. **Device tidak terdeteksi**: Pastikan address I2C benar (LCD: 0x27, PN532: 0x24)
3. **Pin alternatif**: Jika GPIO8/9 bermasalah, coba GPIO18/19

### WiFi Issues
1. **Connection failed**: Periksa kredensial WiFi dan signal strength
2. **WiFiManager timeout**: Device akan membuat AP "SantriCardReader" untuk konfigurasi

## Security Notes

- OTA menggunakan basic authentication dengan kredensial default
- Pertimbangkan mengubah kredensial untuk deployment production
- Pastikan device berada di network yang aman saat OTA

## Version History

- **v1.0.0**: Initial release dengan OTA support
- **Hardware**: ESP32-S3 DevKitC-1
- **Features**: NFC reading, WiFi connectivity, OTA updates, LCD interface