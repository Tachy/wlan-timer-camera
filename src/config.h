#pragma once

// ── WLAN ─────────────────────────────────────────────────────────────────────
#define WIFI_SSID           "MeinNetzwerk"
#define WIFI_PASSWORD       "MeinPasswort"
#define WIFI_TIMEOUT_MS     20000

// ── SCP-Ziel ─────────────────────────────────────────────────────────────────
// Privater SSH-Schlüssel: siehe src/ssh_key.h (nicht ins Repository!)
#define SCP_HOST            "192.168.1.100"
#define SCP_PORT            22
#define SCP_USER            "kamera"
#define SCP_REMOTE_DIR      "/home/kamera/bilder/"
#define SCP_TIMEOUT_SEC     30

// ── Hardware-Pins (ESP32-CAM AI-Thinker) ─────────────────────────────────────
#define PIN_DONE            2   // GPIO 2 → DONE-Signal an TPL5110
#define PIN_FLASH_LED       4   // interne Blitz-LED (aktiv HIGH)

// ── Kamera-Einstellungen ─────────────────────────────────────────────────────
#define CAM_FRAME_SIZE      FRAMESIZE_UXGA   // 1600 × 1200
#define CAM_JPEG_QUALITY    12               // 0–63, niedriger = besser

// ── NVS-Namespace für Boot-Zähler ────────────────────────────────────────────
#define NVS_NAMESPACE       "cam"
#define NVS_KEY_COUNTER     "boot_cnt"
