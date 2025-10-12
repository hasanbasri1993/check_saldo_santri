#!/bin/sh

# ==============================================================================
# A script to initialize and upload a firmware file to a specific server.
#
# Proses:
# 1. Mengirim permintaan inisialisasi (start) ke server.
# 2. Mengunggah file firmware.
# ==============================================================================

# --- Konfigurasi ---
HOST="http://192.168.87.97:8080"
FIRMWARE_HASH="441018525208457705bf09a8ee3c1093"

# Path ke file firmware, secara dinamis mencari di folder Downloads pengguna.
FIRMWARE_FILE="$HOME/Downloads/firmware.bin"

# --- URL Endpoints ---
START_URL="${HOST}/ota/start?mode=fr&hash=${FIRMWARE_HASH}"
UPLOAD_URL="${HOST}/ota/upload"
# ---------------------

# A. Periksa apakah file firmware ada.
echo "🔎 Memeriksa file firmware: $FIRMWARE_FILE"
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "❌ Error: File firmware tidak ditemukan di '$FIRMWARE_FILE'"
    exit 1
fi

# Langkah 1: Inisialisasi proses OTA (Over-The-Air).
echo "\n▶️  Langkah 1: Memulai proses OTA di server..."
curl "$START_URL" --insecure
echo "\n" # Menambah baris baru agar rapi

# Langkah 2: Unggah file firmware.
echo "🚀 Langkah 2: Mengunggah $FIRMWARE_FILE ke $UPLOAD_URL..."
curl "$UPLOAD_URL" \
  -H 'Accept: */*' \
  -H 'Accept-Language: en-US,en;q=0.9,id;q=0.8' \
  -H 'Cache-Control: no-cache' \
  -H 'Connection: keep-alive' \
  -H 'Origin: http://192.168.87.97:8080' \
  -H 'Pragma: no-cache' \
  -H 'Referer: http://192.168.87.97:8080/update' \
  -H 'User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36' \
  -F "file=@$FIRMWARE_FILE;type=application/macbinary" \
  --insecure

# Pesan Selesai.
echo "\n✅ Proses selesai."