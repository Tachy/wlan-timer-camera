# Schaltplanbeschreibung 

funktioniert direkt als Eingabe für den Claude-Skill "schematic-diagram", der den Schaltplan rendert.
https://github.com/Tachy/claude-skills

---

### 1. BAUTEILELISTE (COMPONENTS)

Der Aufbau ist in vier physische Module gegliedert. Jedes Modul kommuniziert ausschließlich über seine deklarierten Außenpins mit den anderen.

**Modul A — Stromversorgung**
- PSU1: 12 V Netzteil / Konstantspannung
- U2: QUARKZMAN Step-Down Wandler | 12 V → 3,3 V / 3 A / 10 W | IP67 | Ausgangsspannung fest eingestellt
- Außenpins: `3,3 V` (VOUT), `GND`

**Modul B — Vorschaltplatine (Streifenrasterplatine mit TPL5110)**
- U1: TPL5110 Power-Timer IC | Gehäuse: SOT-23-6 oder generischer 5-Pin-Block
- C2: Pufferkondensator | Elektrolytkondensator | 4700 µF | parallel zu VDD/GND
- R1: Timer-Widerstand | 47 kΩ | zwischen U1 DELAY und GND | legt Einschaltdauer auf 6,5 Minuten fest
- R2: DONE-Pull-down | 10 kΩ | zwischen U1 DONE und GND | verhindert floatenden Eingang
- Außenpins: `VDD`, `GND`, `DRV (Pin 4)`, `DONE (Pin 5)`
- DELAY wird intern verdrahtet — kein Außenpin

**Modul C — AO3400 Breakoutplatine**
- Q1: AO3400 N-Kanal MOSFET | Gehäuse: SOT-23 | Low-Side-Switch
- Außenpins: `Pin 1 (Gate)`, `Pin 2 (Source)`, `Pin 3 (Drain)`

**Modul D — Recheneinheit & Kamera**
- M1: ESP32-CAM (AI-Thinker) mit OV2640 | integrierte Blitz-LED an GPIO 4
- C1: Glättungskondensator (Low-ESR) | 470 µF / 6,3 V | parallel zur Kamera | intern verdrahtet
- Außenpins: `3,3 V`, `GND (geschaltet)`, `GPIO 13 (DONE)`

---

### 2. VERDRAHTUNGSVORSCHRIFT (MODUL-ZU-MODUL / PIN-TO-PIN)

Verbinde die Module ausschließlich über ihre Außenpins. Alle internen Verbindungen sind je Modul separat aufgeführt.

---

#### A → B: Versorgung Vorschaltplatine

| Von (Modul A) | Nach (Modul B) | Netz |
|---|---|---|
| `3,3 V` | `VDD` | VCC_3V3 |
| `GND` | `GND` | GND (sternförmig) |

#### A → C: GND zum MOSFET (sternförmig, direkt)

| Von (Modul A) | Nach (Modul C) | Netz |
|---|---|---|
| `GND` | `Pin 2 (Source)` | GND (sternförmig) |

> **Hinweis:** GND wird sternförmig von `psu_GND` verteilt — direkt zur Vorschaltplatine (`tpl_GND`) UND direkt zum MOSFET-Breakout (`q1_p2`), ohne Kette durch die Vorschaltplatine.

#### A → D: 3,3 V zur Kamera (Dauerpuls, direkt)

| Von (Modul A) | Nach (Modul D) | Netz |
|---|---|---|
| `3,3 V` | `3,3 V` | VCC_3V3 (Dauerpuls) |

> **Wichtig:** M1 wird dauerhaft mit 3,3 V versorgt — auch wenn der MOSFET sperrt. Der MOSFET schaltet nur die Masse (Low-Side-Switch).

#### B → C: Timer-Steuersignal

| Von (Modul B) | Nach (Modul C) | Netz |
|---|---|---|
| `DRV (Pin 4)` | `Pin 1 (Gate)` | TPL_DRV |

> TPL5110 zieht DRV im Schlaf aktiv auf GND → MOSFET sicher AUS.

#### C → D: Geschaltete Masse

| Von (Modul C) | Nach (Modul D) | Netz |
|---|---|---|
| `Pin 3 (Drain)` | `GND (geschaltet)` | GND_SWITCHED |

> Kamera bekommt Masse erst wenn der MOSFET durchschaltet.

#### D → B: Abschaltsignal (Feedback)

| Von (Modul D) | Nach (Modul B) | Netz |
|---|---|---|
| `GPIO 13 (DONE)` | `DONE (Pin 5)` | MCU_DONE |

> ESP32-CAM signalisiert dem TPL5110, dass die Aufnahme abgeschlossen ist → Timer schaltet ab.

---

#### Interne Verdrahtung Modul A (Stromversorgung)

- PSU1 `+` → U2 `VIN` (12 V)
- U2 `VOUT` → Außenpin `3,3 V`
- PSU1 `−` → Außenpin `GND`
- U2 `GND` → Außenpin `GND`

#### Interne Verdrahtung Modul B (Vorschaltplatine)

- `VDD`-Pin → U1 `VDD`, C2 `+`
- `GND`-Pin → interne GND-Ebene (U1 `GND`, C2 `−`, R1 `Pin 2`, R2 `Pin 2`)
- U1 `DELAY` → R1 `Pin 1` *(TPL_DELAY, kein Außenpin)*
- R2 `Pin 1` → `DONE (Pin 5)`-Pin *(10 kΩ Pull-down hält DONE auf GND solange MCU schweigt)*
- `DONE (Pin 5)`-Pin → U1 `DONE`
- U1 `DRV` → `DRV (Pin 4)`-Pin

#### Interne Verdrahtung Modul C (MOSFET-Breakout)

- `Pin 1 (Gate)` → Q1 Gate
- `Pin 2 (Source)` → Q1 Source
- Q1 Drain → `Pin 3 (Drain)`

#### Interne Verdrahtung Modul D (Recheneinheit & Kamera)

- `3,3 V`-Pin → M1 `3,3V`, C1 `+`
- `GND (geschaltet)`-Pin → M1 `GND`, C1 `−`
- M1 `GPIO 13` → `GPIO 13 (DONE)`-Pin
