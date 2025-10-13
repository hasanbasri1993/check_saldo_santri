#!/bin/sh

# ==============================================================================
# Skrip untuk build dan upload firmware, dengan host yang dapat diubah.
#
# Penggunaan:
#   ./upload.sh [optional_host_url]
# ==============================================================================

# --- Konfigurasi ---
DEFAULT_HOST="http://192.168.87.97:8080"
# Kredensial Basic Auth default (sesuaikan bila perlu)
DEFAULT_USERNAME="admin"
DEFAULT_PASSWORD="santri123"

# Gunakan argumen pertama ($1) sebagai HOST jika ada, jika tidak, gunakan default.
# Opsional: argumen kedua ($2) = username, ketiga ($3) = password.
HOST="${1:-$DEFAULT_HOST}"
USERNAME="${2:-$DEFAULT_USERNAME}"
PASSWORD="${3:-$DEFAULT_PASSWORD}"

FIRMWARE_HASH="441018525208457705bf09a8ee3c1093"

# --- URL Endpoints ---
START_URL="${HOST}/ota/start?mode=fr&hash=${FIRMWARE_HASH}"
UPLOAD_URL="${HOST}/ota/upload"
# ---------------------

echo "🔌 Menggunakan host: $HOST"
echo "🔐 Basic Auth: $USERNAME:********"

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
curl --connect-timeout 5 -u "$USERNAME:$PASSWORD" "$START_URL" --insecure
echo "\n"

# Langkah 4: Unggah file firmware
echo "🚀 Langkah 4: Mengunggah $FIRMWARE_FILE..."
curl -u "$USERNAME:$PASSWORD" "$UPLOAD_URL" \
  -H 'Accept: */*' \
  -H "Origin: ${HOST}" \
  -H "Referer: ${HOST}/update" \
  -F "file=@$FIRMWARE_FILE;type=application/macbinary" \
  --insecure

# Pesan Selesai
echo "\n✅ Proses selesai."