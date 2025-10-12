#!/bin/sh

# ==============================================================================
# Skrip untuk build dan upload firmware, dengan host yang dapat diubah.
#
# Penggunaan:
#   ./upload.sh [optional_host_url]
# ==============================================================================

# --- Konfigurasi ---
DEFAULT_HOST="http://192.168.87.97:8080"
# Gunakan argumen pertama ($1) sebagai HOST jika ada, jika tidak, gunakan default.
HOST="${1:-$DEFAULT_HOST}"

FIRMWARE_HASH="441018525208457705bf09a8ee3c1093"

# --- URL Endpoints ---
START_URL="${HOST}/ota/start?mode=fr&hash=${FIRMWARE_HASH}"
UPLOAD_URL="${HOST}/ota/upload"
# ---------------------

echo "🔌 Menggunakan host: $HOST"

# Langkah 1: Build proyek PlatformIO menggunakan environment default
echo "▶️  Langkah 1: Membangun firmware..."
platformio run

# Periksa apakah build berhasil
if [ $? -ne 0 ]; then
    echo "❌ Error: Build PlatformIO gagal. Proses dihentikan."
    exit 1
fi
echo "✅ Build berhasil."

# Langkah 2: Cari file firmware.bin yang paling baru
echo "🔎 Mencari file firmware.bin..."
FIRMWARE_FILE=$(find .pio/build -name firmware.bin -print0 | xargs -0 ls -t | head -n 1)

if [ -z "$FIRMWARE_FILE" ] || [ ! -f "$FIRMWARE_FILE" ]; then
    echo "❌ Error: Tidak dapat menemukan file firmware.bin setelah build."
    exit 1
fi
echo "👍 Ditemukan: $FIRMWARE_FILE"

# Langkah 3: Inisialisasi proses OTA
echo "\n▶️  Langkah 3: Memulai proses OTA..."
curl --connect-timeout 5 "$START_URL" --insecure
echo "\n"

# Langkah 4: Unggah file firmware
echo "🚀 Langkah 4: Mengunggah $FIRMWARE_FILE..."
curl "$UPLOAD_URL" \
  -H 'Accept: */*' \
  -H "Origin: ${HOST}" \
  -H "Referer: ${HOST}/update" \
  -F "file=@$FIRMWARE_FILE;type=application/macbinary" \
  --insecure

# Pesan Selesai
echo "\n✅ Proses selesai."