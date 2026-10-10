# BallyLa_60 – Lamp Driver Replacement Firmware

Firmware (PIC18F25K42, XC8) für das **BallyLA_60 Replacement Board**
(<https://lisy.dev/ballyla_60.html>). Sie ersetzt die original Bally **2518-14
Lampdriver-Platine** (Thyristoren) durch eine MOSFET-Lösung: Die original
Bally-Ansteuerung (Adresse + Inhibit/Daten + Strobe) wird am Steckverbinder
eingelesen und daraus die 60-Lampen-Matrix angesteuert.

Quellen: `main.c`, `def.h`.

---

## Funktionsprinzip (ab v006)

- **INT0 (RB0 / Strobe, fallende Flanke):** Adresse aus `PORTA & 0x0F` lesen,
  dann warten, bis die Adressleitungen auf 15 gehen (Datenphase), und dort das
  Inhibit-/Datenwort (`PORTA` High-Nibble) lesen. Jede Lampe mit Inhibit-Bit = 0
  wird in `lamp_int[]` gesetzt (Akkumulator).
- **Timer2 (freilaufend, ~13 ms):** Übernimmt `lamp_int[]` nach `lamp[]` und
  löscht den Akkumulator. Emuliert das Thyristor-Verhalten (Lampe bleibt bis zum
  nächsten Nulldurchgang an; Bally refresht jeden Nulldurchgang, 10 ms @ 50 Hz).
- **Hauptschleife / `update_ff()`:** Bei Änderung werden die 8 Flip-Flops
  (74HC574) über `LATC`/`A0..A2` neu beschrieben.
- **IOC (RB6/RB7):** Tasten S1/S2 für den Testmodus.

---

## Befund aus dem Logic-Analyzer-Scan (`lamps.LPF`)

LogicPort-Aufzeichnung, **Sample-Periode 100 ns**. Das `.LPF` ist
lauflängencodiert; die Spalte `Count` = Anzahl 100-ns-Samples, die ein Zustand
anhält.

### Kanal-Zuordnung (an `PORTA` des PIC)

| Kanal       | Bedeutung                                            |
|-------------|------------------------------------------------------|
| **D0**      | STROBE (RB0 / INT0), aktiv-HIGH-Puls, ~10 µs breit   |
| **D8–D11**  | Lampen-Adresse (RA0–RA3), 4 Bit binär, D8 = LSB      |
| **D12–D15** | Inhibit/Daten (RA4–RA7), **0 = Lampe AN**            |

### Busprotokoll: zwei Phasen pro Adresse

```
Phase 1  addr = A,  data = 1111 (alle AUS)   ~28 µs, Strobe-Puls mittendrin
                                             (latcht die Adresse in die 4514)
Phase 2  addr = 15, data = Lampendaten       ~24 µs   <- nur hier stehen die Daten
```

Auswertung aller 144 fallenden Strobe-Flanken im Scan:

- Daten **an der Flanke: 144 × `1111`**, also immer „alle aus“.
- Echte Daten liegen von **+13,1 µs bis +37,3 µs** nach der fallenden Flanke an,
  genau solange die Adressleitungen auf 15 stehen.
- Zwischen Adress- und Datenleitungen gibt es beim Phasenwechsel ~1 Sample Skew.
- Strobe-Periode ~62,6 µs, voller Scan ~1,5 ms, Wiederholung ~alle 10 ms.

---

## Versionsgeschichte des Lesezeitpunkts

| Version | Lesezeitpunkt Daten        | Ergebnis |
|---------|----------------------------|----------|
| v003    | fest +20 µs nach Flanke    | trifft hier das Fenster 13–37 µs; bei anderer MPU-Timing-Lage evtl. nicht → vermutlich „Fehler bei einigen Flippern“ |
| v004/v005 | +3 µs nach Flanke        | **defekt**: liest immer `1111`, keine Lampe geht an (nur Testmodus funktioniert) |
| v006    | wenn Adresse = 15 (+2 µs Einschwingzeit) | an das Protokoll gekoppelt, unabhängig vom Maschinen-Timing |

Die Analyse zu v004 hatte angenommen, die Daten seien schon an der Strobe-Flanke
gültig, und den Wert an der Flanke als Referenz genommen. Dadurch sah das
Lesen bei +3 µs fälschlich „105/105 korrekt“ aus.

### Lesepfad v006 (`Int0_Isr`)

```c
address = PORTA & 0x0f;                 // Adresse, stabil bis ~13 µs nach Flanke
if (address != 15) {
    timeout = DATA_PHASE_TIMEOUT;       // 50 µs
    while ((PORTA & 0x0f) != 0x0f) {    // auf Datenphase warten
        if (--timeout == 0) return;     // keine Datenphase -> Strobe verwerfen
        __delay_us(1);
    }
    __delay_us(2);                      // Skew Adress-/Datenleitungen abwarten
    porta = PORTA;
    if ((porta & 0x0f) != 0x0f) return; // Bus schon weiter -> verwerfen
    ... lamp_int[] aus porta-Bits 4..7 setzen
}
```

Re-Simulation gegen `lamps.LPF` (0,5 µs ISR-Latenz angenommen): alle 105
Strobes mit Adresse ≠ 15 lesen den Wert der Datenphase.

---

## Bekannte Annahmen / offene Punkte

- Das v005-Refresh-Modell (freilaufend 13 ms, Akkumulator) ist erst ab v006
  real testbar, weil v005 wegen des Lesefehlers nie Daten bekam. Bei Flackern:
  Rückfall auf den v003-Refresh (One-Shot 2 ms nach Adresse 0) mit dem neuen
  Lesepfad.
- Der CMake-Build (VSCode) setzt `__DEBUG=1`; für Release-Hexe ggf. eine
  Production-Konfiguration verwenden.

---

## Verifikation / Test

1. **Build:** in VSCode (XC8 v3.10, PIC18F25K42) → `out/BallyLa_60/default.hex`.
2. **Teststand mit CPU:** Versionsanzeige „6“, danach Attract-Lampen sichtbar;
   Testmodus weiterhin ok.
3. **Maschinen:** KISS (4x4-Attract-Matrix) und eine bisher problematische Maschine.
