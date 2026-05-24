# Schaltplanbeschreibung 

funktioniert direkt als Eingabe für den Claude-Skill "schematic-diagram", der den Schaltplan rendert.
https://github.com/Tachy/claude-skills

---

### 1. BAUTEILELISTE (COMPONENTS)

- BAT1: Primärzelle | Typ: ER26500 Lithium-Thionylchlorid (Li-SOCl2) | Wert: 3.6V, 8500 mAh
- U1: Power-Timer | Typ: TPL5110 (Breakout/IC) | Gehäuse: SOT-23-6 oder generischer 5-Pin-Block
- Q1: Hauptschalter | Typ: Si2301 P-Kanal MOSFET | Gehäuse: SOT-23
- R1: Gate-Pull-up | Typ: Widerstand | Wert: 100 kΩ (zieht Gate im Ruhezustand auf VCC_BATT → MOSFET sicher AUS)
- C1: Glättungskondensator | Typ: Elektrolytkondensator (Low-ESR) | Wert: 470 µF / 6.3V (Platziert NACH dem MOSFET)
- M1: Recheneinheit & Kamera | Typ: ESP32-CAM Modul (inkl. OV2640 & integrierter Blitz-LED auf GPIO 4)

---

### 2. VERDRAHTUNGSVORSCHRIFT (NETLIST / PIN-TO-PIN)

Verbinde die Pins der Bauteile exakt nach folgenden elektrischen Netzen (Nets):

#### Netz: GND (Globale Masse)
- BAT1 (Minuspol / -)
- U1 (Pin GND)
- C1 (Minuspol / -)
- M1 (Pin GND)

#### Netz: VCC_BATT (Ungeschaltete Batteriespannung)
- BAT1 (Pluspol / +)
- U1 (Pin VDD)
- Q1 (Pin Source)
- R1 (Pin 1)

#### Netz: TPL_DRV (Timer-Steuersignal)
- U1 (Pin DRV)
- Q1 (Pin Gate)
- R1 (Pin 2)  <-- Pull-up hält Gate auf VCC_BATT wenn DRV hochohmig (MOSFET AUS)

#### Netz: VCC_SWITCHED (Geschaltete Versorgungsleitung - HIER SITZT DER ELKO)
- Q1 (Pin Drain)
- C1 (Pluspol / +)
- M1 (Pin 3.3V)  <-- WICHTIG: Direkt an 3.3V, nicht an den 5V-Pin!

#### Netz: MCU_DONE (Abschaltsignal)
- M1 (Pin GPIO 13)
- U1 (Pin DONE)
