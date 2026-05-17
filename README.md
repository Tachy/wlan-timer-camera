# WLAN Timer-Kamera

ESP32-CAM Intervallfotografie mit TPL5110-gesteuertem Power-Gating.
Alle 15 Minuten wird ein Bild aufgenommen und per SCP auf einen Server übertragen.
Die MCU läuft nur während der Aufnahme — die Stromversorgung übernimmt der Hardware-Timer.

## Funktionsprinzip

```
TPL5110 ──(DRV)──► Si2301 P-MOSFET ──► VCC_SWITCHED ──► ESP32-CAM (3,3 V-Pin)
                                                               │
                         TPL5110 ◄──(DONE, GPIO 2)─────────────┘
```

1. TPL5110 schaltet alle 15 Minuten den MOSFET durch → ESP32-CAM startet
2. Firmware läuft durch: NVS-Zähler → Kamera → WLAN → SCP-Upload
3. GPIO 2 wird auf HIGH gezogen → TPL5110 trennt die Stromversorgung

## Hardware

| Bauteil | Wert / Typ |
|---|---|
| Batterie | ER26500, Li-SOCl₂, 3,6 V / 8500 mAh |
| Timer | TPL5110 (SOT-23-6) |
| Intervallwiderstand | R1 = 100 kΩ → 15 min |
| Schalttransistor | Si2301, P-Kanal MOSFET (SOT-23) |
| Glättungskondensator | C1 = 470 µF / 6,3 V, Low-ESR |
| MCU + Kamera | ESP32-CAM (AI-Thinker) mit OV2640 |

**Achtung:** Der ESP32-CAM wird am **3,3-V-Pin** betrieben, nicht am 5-V-Pin.

### Pin-Belegung ESP32-CAM

| Signal | GPIO |
|---|---|
| DONE → TPL5110 | GPIO 2 |
| Blitz-LED (intern) | GPIO 4 |
| Kamera-Pins | festgelegt in `src/camera.c` |

## Voraussetzungen

- [PlatformIO](https://platformio.org/) (CLI oder VS Code Extension)
- ESP-IDF ≥ 5.0 (wird von PlatformIO automatisch geladen)
- libssh2-Komponente für ESP-IDF (siehe unten)

## Einrichtung

### 1. Repository klonen

```bash
git clone <repo-url> wlan-timer-camera
cd wlan-timer-camera
```

### 2. libssh2-Komponente

`skuodi/libssh2_esp` ist in `src/idf_component.yml` eingetragen und wird beim ersten
Build automatisch vom IDF Component Manager heruntergeladen. Kein manueller Schritt nötig.

### 3. SSH-Schlüsselpaar erzeugen

```bash
ssh-keygen -t ed25519 -f ~/.ssh/kamera -C "esp32cam" -N ""
```

Den öffentlichen Schlüssel auf dem Zielserver eintragen:

```bash
cat ~/.ssh/kamera.pub >> /home/kamera/.ssh/authorized_keys
chmod 600 /home/kamera/.ssh/authorized_keys
```

### 4. Schlüssel in die Firmware einbetten

Datei `src/ssh_key.h` anlegen (wird nicht eingecheckt):

```bash
cp src/ssh_key.h.example src/ssh_key.h   # falls vorhanden, sonst manuell anlegen
```

Inhalt von `~/.ssh/kamera` in `SSH_PRIVATE_KEY` einfügen:

```c
#pragma once

#define SSH_PRIVATE_KEY \
    "-----BEGIN OPENSSH PRIVATE KEY-----\n" \
    "b3BlbnNzaC1rZXktdjEAAAAABG5v...\n" \
    "-----END OPENSSH PRIVATE KEY-----\n"

#define SSH_KEY_PASSPHRASE  ""
```

Jede Zeile des Base64-Blocks als eigenes String-Literal mit `\n` und `\` abschließen.

### 5. Konfiguration anpassen

`src/config.h` öffnen und folgende Werte setzen:

```c
#define WIFI_SSID        "MeinNetzwerk"
#define WIFI_PASSWORD    "MeinPasswort"

#define SCP_HOST         "192.168.1.100"   // IP oder Hostname des Zielservers
#define SCP_USER         "kamera"
#define SCP_REMOTE_DIR   "/home/kamera/bilder/"
```

Bildqualität und Auflösung bei Bedarf anpassen:

```c
#define CAM_FRAME_SIZE   FRAMESIZE_UXGA   // 1600 × 1200
#define CAM_JPEG_QUALITY 12               // 0–63, niedriger = besser
```

### 6. Firmware bauen und flashen

```bash
pio run --target upload
```

Serielle Ausgabe beobachten:

```bash
pio device monitor
```

## Projektstruktur

```
├── platformio.ini          PlatformIO-Konfiguration
├── sdkconfig.defaults      ESP-IDF Standardwerte (PSRAM, Stack, lwIP)
├── .gitignore
├── components/
│   └── libssh2/            SSH2-Bibliothek (manuell hinzufügen, s. o.)
└── src/
    ├── config.h            Alle konfigurierbaren Parameter
    ├── ssh_key.h           Privater SSH-Schlüssel (nicht im Repo!)
    ├── main.c              Ablaufsteuerung
    ├── camera.c / .h       OV2640-Initialisierung und Bildaufnahme
    ├── wifi_conn.c / .h    WLAN-Verbindung
    ├── scp_upload.c / .h   SCP-Upload via libssh2
    └── idf_component.yml   ESP-IDF-Komponentenabhängigkeiten
```

## Ablaufdiagramm Firmware

```
app_main()
  │
  ├─ NVS-Zähler lesen → Dateiname img_000042.jpg
  ├─ Kamera initialisieren
  ├─ 500 ms warten (Sensor-Stabilisierung)
  ├─ Bild aufnehmen (JPEG, PSRAM)
  ├─ WLAN verbinden (Timeout: 20 s)
  ├─ SCP-Upload
  │    TCP connect → SSH-Handshake → Key-Auth → scp_send64 → Daten → EOF
  ├─ WLAN trennen
  └─ GPIO 2 HIGH (200 ms) → TPL5110 schaltet Strom ab
```

Bei jedem Fehler (Kamera, WLAN, Upload) wird das DONE-Signal trotzdem gesendet,
damit das System nicht dauerhaft eingeschaltet bleibt.

## Dateinamen

Bilder werden fortlaufend nummeriert: `img_000000.jpg`, `img_000001.jpg`, …  
Der Zähler wird im NVS (Flash) gespeichert und überlebt einen Neustart.

## Stromverbrauch (Richtwerte)

| Phase | Dauer | Strom |
|---|---|---|
| Schlafen (TPL5110 aktiv, MOSFET aus) | ~14:55 min | < 1 µA |
| Boot + Kamera-Init | ~1 s | ~150 mA |
| WLAN-Verbindung + Upload | ~5–15 s | ~200 mA |
| Gesamt pro Zyklus (15 min) | — | ~Ø 3–5 µA |

Mit der ER26500 (8500 mAh) ergibt sich eine rechnerische Laufzeit von mehreren Jahren.
