# WLAN Timer-Kamera

ESP32-CAM Intervallfotografie mit TPL5110-gesteuertem Power-Gating.
Alle 15 Minuten wird ein Bild aufgenommen und per SCP auf einen Server übertragen.
Die MCU läuft nur während der Aufnahme — die Stromversorgung übernimmt der Hardware-Timer.

## Funktionsprinzip

```
TPL5110 ──(DRV)──► Si2301 P-MOSFET ──► VCC_SWITCHED ──► ESP32-CAM (3,3 V-Pin)
                                                               │
                         TPL5110 ◄──(DONE, GPIO 13)────────────┘
```

1. TPL5110 schaltet alle 15 Minuten den MOSFET durch → ESP32-CAM startet
2. Firmware läuft durch: NVS-Zähler → Kamera → WLAN → SCP-Upload
3. GPIO 13 wird auf HIGH gezogen → TPL5110 trennt die Stromversorgung

## Hardware

| Bauteil | Wert / Typ |
|---|---|
| Batterie | ER26500, Li-SOCl₂, 3,6 V / 8500 mAh |
| Timer | TPL5110 (SOT-23-6) |
| Intervallwiderstand | R1 = 100 kΩ → 15 min |
| Schalttransistor | Si2301, P-Kanal MOSFET (SOT-23) |
| Glättungskondensator | C1 = 470 µF / 6,3 V, Low-ESR |
| MCU + Kamera | ESP32-CAM (AI-Thinker) mit OV2640, 8 MB PSRAM, 4 MB Flash |

**Achtung:** Der ESP32-CAM wird am **3,3-V-Pin** betrieben, nicht am 5-V-Pin.

### Pin-Belegung ESP32-CAM

| Signal | GPIO |
|---|---|
| DONE → TPL5110 | GPIO 13 |
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

PEM-Datei anlegen und 1:1 den Inhalt von `~/.ssh/kamera` hineinkopieren:

```bash
# Inhalt von ~/.ssh/kamera nach src/ssh_private_key.pem einfügen — keine Umformatierung nötig
cat ~/.ssh/kamera > src/ssh_private_key.pem
```

Der Linker bettet die Datei beim Build automatisch ein (`board_build.embed_txtfiles`).

### 5. Konfiguration anpassen

WLAN-Zugangsdaten in `src/wlan_keys.h` eintragen (wird nicht eingecheckt):

```bash
cp src/wlan_keys.h.example src/wlan_keys.h
```

```c
#define WIFI_SSID      "MeinNetzwerk"
#define WIFI_PASSWORD  "MeinPasswort"
```

SCP-Ziel in `src/config.h` anpassen:

```c
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

Einmalig: usbipd installieren (als Administrator):

```Powershell
winget install dorssel.usbipd-win
```

Board anstecken und durchrouten (als Administrator):

```Powershell
usbipd list
usbipd attach --wsl --busid <Deine-ID>
```

Prüfen, ob Port durchgeroutet wird:

```bash
lsusb
```

Board programmieren:

```bash
pio run --target upload
```

Serielle Ausgabe beobachten:

```bash
pio device monitor
```


## Projektstruktur

```
├── .gitignore
├── README.md
├── platformio.ini              PlatformIO-Konfiguration
├── sdkconfig.defaults          ESP-IDF Standardwerte (Flash, PSRAM, Stack, lwIP)
├── huge_app.csv                Partitionstabelle (4 MB, App 3 MB)
├── docs/
│   ├── schaltplanbeschreibung.md
│   └── esp32cam_tpl5110.dot / .svg / .png
└── src/
    ├── config.h                Alle konfigurierbaren Parameter (SCP-Ziel, Pins, …)
    ├── wlan_keys.h.example     Vorlage WLAN-Zugangsdaten → wlan_keys.h (gitignored)
    ├── ssh_private_key.pem.example  Vorlage SSH-Schlüssel → ssh_private_key.pem (gitignored)
    ├── idf_component.yml       ESP-IDF-Komponentenabhängigkeiten (Kamera, libssh2)
    ├── main.c                  Ablaufsteuerung
    ├── camera.c / .h           OV2640-Initialisierung, Bildaufnahme mit Blitz-LED
    ├── wifi_conn.c / .h        WLAN-Verbindung
    └── scp_upload.c / .h       SCP-Upload via libssh2
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
  └─ GPIO 13 HIGH (200 ms) → TPL5110 schaltet Strom ab
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

> **Hinweis:** Das ESP32-CAM-Modul hat laut Datenblatt einen Tiefschlafstrom von mindestens 6 mA.
> Dieser Wert ist für unser Design irrelevant, da der TPL5110 die Stromversorgung vollständig
> trennt — der Schlafstrom entspricht dem Ruhestrom des TPL5110 (< 1 µA).
