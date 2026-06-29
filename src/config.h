#pragma once

// ── WLAN ─────────────────────────────────────────────────────────────────────
// SSID und Passwort kommen aus src/wlan_keys.h (nicht im Repo).
// Vorlage: cp src/wlan_keys.h.example src/wlan_keys.h
#include "wlan_keys.h"
#define WIFI_TIMEOUT_MS     45000

// ── Bild-Dateiname ────────────────────────────────────────────────────────────
#define IMAGE_FILENAME      "esp32cam_image.jpg"
#define LOG_FILENAME        "esp32cam_image.log"

// ── SCP-Ziel ─────────────────────────────────────────────────────────────────
// Privater SSH-Schlüssel: src/ssh_private_key.pem (nicht im Repo, s. .gitignore)
// Passphrase leer lassen, wenn der Schlüssel ohne Passphrase erzeugt wurde.
#define SSH_KEY_PASSPHRASE  ""
#define SCP_HOST            "192.168.179.4"
#define SCP_PORT            29876
#define SCP_USER            "apache"
#define SCP_REMOTE_DIR      "/var/www/html/img/"
#define SCP_TIMEOUT_SEC     60

// ── Hardware-Pins (ESP32-CAM AI-Thinker) ─────────────────────────────────────
#define PIN_DONE            13  // GPIO 13 → DONE-Signal an TPL5110
#define PIN_FLASH_LED       4   // interne Blitz-LED (aktiv HIGH)

// ── Kamera-Einstellungen ─────────────────────────────────────────────────────
#define CAM_FRAME_SIZE      FRAMESIZE_UXGA   // 1600 × 1200
#define CAM_JPEG_QUALITY    12               // 0–63, niedriger = besser
