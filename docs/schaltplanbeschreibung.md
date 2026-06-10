# Schaltplanbeschreibung 

funktioniert direkt als Eingabe für den Claude-Skill "schematic-diagram", der den Schaltplan rendert.
https://github.com/Tachy/claude-skills

---

### 1. BAUTEILELISTE (COMPONENTS)

- PSU1: Spannungsversorgung | Typ: 12V Netzteil / Konstantspannung
- U2: Spannungsregler | Typ: QUARKZMAN Step-Down Wandler (DC 12V → DC 3,3V, 3A / 10W, IP67) | Eingangsspannung: 12V fest | Ausgangsspannung: 3,3V fest | Max. Ausgangsstrom: 3A
- C1: Glättungskondensator | Typ: Elektrolytkondensator (Low-ESR) | Wert: 470 µF / 6,3V | Position: parallel zur Kamera
- M1: Recheneinheit & Kamera | Typ: ESP32-CAM Modul (inkl. OV2640 & integrierter Blitz-LED auf GPIO 4)

---

### 2. VERDRAHTUNGSVORSCHRIFT (NETLIST / PIN-TO-PIN)

Verbinde die Pins der Bauteile exakt nach folgenden elektrischen Netzen (Nets):

#### Netz: GND (Globale Masse)
- PSU1 (Minus / -)
- U2 (GND)
- C1 (Minuspol / -)
- M1 (Pin GND)

#### Netz: VCC_12V (Eingangsversorgung)
- PSU1 (Plus / +)
- U2 (VIN)

#### Netz: VCC_3V3 (Geregelte 3,3V Versorgung)
- U2 (VOUT)
- C1 (Pluspol / +)
- M1 (Pin 3.3V)  <-- WICHTIG: Direkt an 3,3V-Pin, nicht an 5V-Pin!
