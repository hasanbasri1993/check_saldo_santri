# Santri Card Reader dengan OTA Update

Sistem pembaca kartu santri berbasis ESP32 dengan fitur Over-The-Air (OTA) update untuk kemudahan maintenance tanpa perlu koneksi USB.

## Fitur Utama

- **Pembacaan Kartu NFC**: Menggunakan PN532 untuk membaca kartu santri dengan format NDEF
- **WiFi Manager**: Konfigurasi WiFi melalui captive portal
- **OTA Update**: Update firmware melalui web interface tanpa USB dengan progress display
- **LCD Display**: Interface 16x2 untuk status dan informasi dengan scrolling text
- **Buzzer Feedback**: Notifikasi audio untuk berbagai state
- **Toggle Switch Interface**: 3-pin SPDT toggle switch untuk pemilihan institusi
- **LED Indicators**: 3 LED untuk menunjukkan institusi yang aktif
- **Authentication System**: Basic authentication untuk konfigurasi dan OTA
- **Configuration Management**: Pengaturan API URL dan mDNS hostname melalui web interface
- **mDNS Discovery**: Auto-discovery device melalui hostname lokal
- **API Integration**: Komunikasi dengan server untuk validasi dan logging

## Hardware Requirements

- **ESP32-S3 DevKitC-1** (atau kompatibel)
- **PN532 NFC Reader** (I2C interface)
- **LCD 16x2 dengan I2C backpack**
- **3-pin SPDT ON-OFF-ON Toggle Switch**
- **3 LED Indicators** (untuk menunjukkan institusi aktif)
- **Buzzer (aktif/pasif)**
- **Power supply 5V**

## Pin Configuration

| Component | GPIO Pin | Keterangan |
|-----------|----------|------------|
| I2C SDA | GPIO18 | Data line untuk LCD dan PN532 |
| I2C SCL | GPIO19 | Clock line untuk LCD dan PN532 |
| Toggle Switch A | GPIO22 | Position 1 (Institution 1) |
| Toggle Switch B | GPIO23 | Position 3 (Institution 3) |
| LED Institution 1 | GPIO21 | Indicator untuk Institution 1 |
| LED Institution 2 | GPIO4 | Indicator untuk Institution 2 |
| LED Institution 3 | GPIO5 | Indicator untuk Institution 3 |
| Buzzer | GPIO19 | Audio feedback |
| Built-in RGB LED | GPIO48 | WS2812B LED (WeAct Studio board) |

## OTA (Over-The-Air) Update

### Setup OTA
1. OTA berjalan di background setelah WiFi tersambung (tidak mengganggu interface kartu)
2. Serial monitor akan menunjukkan URL OTA: `http://[device_ip]:8080/`
3. mDNS discovery: `http://[hostname].local:8080/` (misalnya: `http://santri-reader.local:8080/`)

### Cara Update Firmware
1. **Buka browser** dan akses:
   - `http://[device_ip]:8080/`
   - atau `http://[hostname].local:8080/`
2. **Klik** tombol "Go to OTA Update"
3. **Login** dengan kredensial (default):
   - Username: `admin`
   - Password: `santri123`
4. **Upload file .bin** hasil build dari PlatformIO
5. **LCD menampilkan progress akurat** berdasarkan ukuran file sebenarnya
6. **Device reboot otomatis** setelah update selesai

### Web Interface Endpoints

| Endpoint | Fungsi | Authentication |
|----------|--------|----------------|
| `http://ip:8080/` | Dashboard device info | Tidak perlu |
| `http://ip:8080/config` | Configuration management | `admin:santri123` |
| `http://ip:8080/mdns-status` | mDNS status dan restart | `admin:santri123` |
| `http://ip:8080/update` | OTA firmware upload | `admin:santri123` |
| `http://ip:8080/info` | JSON device info | Tidak perlu |

### Configuration Management
1. **Akses**: `http://[device_ip]:8080/config`
2. **Login** dengan kredensial default
3. **Ubah**:
   - API Base URL
   - mDNS Hostname
   - Authentication credentials
4. **Device restart otomatis** setelah perubahan konfigurasi

## API Integration

### Card Validation
```http
GET /check?id_card={uid}&id_santri={santri_id}&id_device={mac_address}
```

### Activity Logging
```http
POST /log
Content-Type: multipart/form-data

id_card={uid}&id_santri={santri_id}&institution={1,2,3}
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
    tzapu/WiFiManager@^2.0.17
    adafruit/Adafruit PN532@^1.3.4
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
    bblanchon/ArduinoJson@^7.4.2
    ESP32Async/ESPAsyncWebServer@3.7.3
    ESP32Async/AsyncTCP@3.3.7
    https://github.com/ayushsharma82/ElegantOTA.git
    https://github.com/teknynja/Adafruit_NeoPixel
    ESP32mDNS
```

### Build Flags
```ini
build_flags =
    -D ELEGANTOTA_USE_ASYNC_WEBSERVER=1
    -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000
    -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64
    -D ARDUINOJSON_USE_LONG_LONG=1
    -D CONFIG_PM_ENABLE=0
    -D CONFIG_FREERTOS_USE_TICKLESS_IDLE=0
```

## Troubleshooting

### OTA Issues
1. **OTA tidak muncul**: Periksa WiFi connection dan port 8080
2. **Upload gagal**: Pastikan file .bin valid dan ukuran sesuai flash
3. **Authentication error**: Gunakan username `admin` dan password `santri123`
4. **mDNS tidak berubah**: Restart mDNS service atau clear router cache

### I2C Issues
1. **Error I2C transaction**: Periksa koneksi hardware dan pin configuration
2. **Device tidak terdeteksi**: Pastikan address I2C benar (LCD: 0x27, PN532: 0x24)
3. **Pin alternatif**: Jika GPIO18/19 bermasalah, coba konfigurasi ulang

### WiFi Issues
1. **Connection failed**: Periksa kredensial WiFi dan signal strength
2. **WiFiManager timeout**: Device akan membuat AP untuk konfigurasi WiFi

### Authentication Issues
1. **Login gagal**: Pastikan kredensial benar (default: admin/santri123)
2. **Browser tidak prompt login**: Gunakan mode incognito atau clear cache
3. **Lupa password**: Reset konfigurasi melalui `/config/reset` atau clear EEPROM

### Configuration Issues
1. **API URL tidak tersimpan**: Pastikan format URL benar (http:// atau https://)
2. **mDNS hostname error**: Pastikan hostname hanya alphanumeric dan hyphen
3. **LoadProhibited crash**: Restart device atau clear EEPROM jika corrupt

## Security Notes

- **Basic Authentication** melindungi konfigurasi dan OTA upload
- **Default credentials**: `admin:santri123` - ubah untuk production
- **HTTPS recommended** untuk API komunikasi
- **Network security**: Pastikan device berada di network yang aman
- **EEPROM storage**: Kredensial tersimpan di EEPROM (offset 200-264)

## Version History

- **v2.0.0**: Major update dengan authentication dan configuration management
- **Hardware**: ESP32-S3 DevKitC-1 dengan WeAct Studio board
- **Features**:
  - Authentication system untuk konfigurasi dan OTA
  - mDNS discovery dan configuration management
  - Toggle switch interface dengan LED indicators
  - WS2812B RGB LED support
  - Enhanced error handling dan debugging
  - Performance analysis dan timing reports