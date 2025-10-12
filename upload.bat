@echo off
rem ==============================================================================
rem  Skrip untuk build dan upload firmware, dengan host yang dapat diubah.
rem
rem  Penggunaan:
rem    upload.bat [optional_host_url]
rem ==============================================================================

:: --- Konfigurasi ---
set "DEFAULT_HOST=http://192.168.87.97:8080"
set "HOST=%DEFAULT_HOST%"

:: Jika argumen pertama (%1) diberikan, gunakan sebagai HOST.
if not "%~1"=="" (
    set "HOST=%~1"
)

rem PENTING: Pastikan variabel ini cocok dengan nama environment default Anda.
set "DEFAULT_PIO_ENV=esp32-s3-devkitc-1"

set "FIRMWARE_HASH=441018525208457705bf09a8ee3c1093"
set "FIRMWARE_FILE=%CD%\.pio\build\%DEFAULT_PIO_ENV%\firmware.bin"

:: --- URL Endpoints ---
set "START_URL=%HOST%/ota/start?mode=fr&hash=%FIRMWARE_HASH%"
set "UPLOAD_URL=%HOST%/ota/upload"
:: ---------------------

echo Menggunakan host: %HOST%

:: Langkah 1: Build proyek PlatformIO
echo.
echo Langkah 1: Membangun firmware...
platformio run

:: Periksa apakah build berhasil
if %errorlevel% neq 0 (
    echo.
    echo Error: Build PlatformIO gagal. Proses dihentikan.
    goto :eof
)
echo Build berhasil.

:: Periksa apakah file firmware ada
if not exist "%FIRMWARE_FILE%" (
    echo Error: File firmware tidak ditemukan di '%FIRMWARE_FILE%'.
    echo Pastikan nama DEFAULT_PIO_ENV di skrip ini sudah benar.
    goto :eof
)

:: Langkah 2: Inisialisasi proses OTA
echo.
echo Langkah 2: Memulai proses OTA...
curl --connect-timeout 5 "%START_URL%" --insecure
echo.
echo.

:: Langkah 3: Unggah file firmware
echo Langkah 3: Mengunggah %FIRMWARE_FILE%...
echo.
curl "%UPLOAD_URL%" ^
  -H "Accept: */*" ^
  -H "Origin: %HOST%" ^
  -H "Referer: %HOST%/update" ^
  -F "file=@%FIRMWARE_FILE%;type=application/macbinary" ^
  --insecure

:: Pesan Selesai
echo.
echo.
echo Proses selesai.
echo.