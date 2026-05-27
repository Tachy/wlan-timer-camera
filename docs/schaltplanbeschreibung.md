# Schaltplanbeschreibung 

funktioniert direkt als Eingabe für den Claude-Skill "schematic-diagram", der den Schaltplan rendert.
https://github.com/Tachy/claude-skills

---

### 1. BAUTEILELISTE (COMPONENTS)

- BAT1: Primärzelle | Typ: ER26500 Lithium-Thionylchlorid (Li-SOCl2) | Wert: 3.6V, 8500 mAh
- U1: Power-Timer | Typ: TPL5110 (Breakout/IC) | Gehäuse: SOT-23-6 oder generischer 5-Pin-Block
- Q1: Hauptschalter | Typ: AO3400 N-Kanal MOSFET | Gehäuse: SOT-23
- C1: Glättungskondensator | Typ: Elektrolytkondensator (Low-ESR) | Wert: 470 µF / 6.3V (Platziert parallel zur Kamera, im Block "Recheneinheit & Kamera")
- M1: Recheneinheit & Kamera | Typ: ESP32-CAM Modul (inkl. OV2640 & integrierter Blitz-LED auf GPIO 4)

---

### 2. VERDRAHTUNGSVORSCHRIFT (NETLIST / PIN-TO-PIN)

Verbinde die Pins der Bauteile exakt nach folgenden elektrischen Netzen (Nets):

#### Netz: GND (Globale Masse / Batterie-Minus)
- BAT1 (Minuspol / -)
- U1 (Pin GND)
- Q1 (Pin 2 / Source)

#### Netz: VCC_BATT (Dauerplus / Globale Versorgungsspannung)
- BAT1 (Pluspol / +)
- U1 (Pin VDD)
- C1 (Pluspol / +)
- M1 (Pin 3.3V)  <-- WICHTIG: Dauerhaft an 3.3V, nicht an den 5V-Pin!

#### Netz: TPL_DRV (Timer-Steuersignal)
- U1 (Pin 4 / DRV)
- Q1 (Pin 1 / Gate)  <-- TPL5110 zieht im Schlaf aktiv auf GND (N-Kanal MOSFET sicher AUS)

#### Netz: GND_SWITCHED (Geschaltete Masseleitung - HIER SITZT DER ELKO)
- Q1 (Pin 3 / Drain)
- C1 (Minuspol / -)
- M1 (Pin GND)  <-- Kamera bekommt Masse erst, wenn der MOSFET durchschaltet!

#### Netz: MCU_DONE (Abschaltsignal)
- M1 (Pin GPIO 13)
- U1 (Pin 5 / DONE)