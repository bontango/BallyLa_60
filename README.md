# BallyLa_60 – Lamp Driver Replacement Firmware

Firmware (PIC18F25K42, XC8) für das **BallyLA_60 Replacement Board**
(<https://lisy.dev/ballyla_60.html>). Sie ersetzt die original Bally **2518-14
Lampdriver-Platine** (Thyristoren) durch eine MOSFET-Lösung: Die original
Bally-Ansteuerung (Adresse + Inhibit/Daten + Strobe) wird am Steckverbinder
eingelesen und daraus die 60-Lampen-Matrix angesteuert.

Quellen: `main.c`, `def.h`.

---

## Funktionsprinzip

- **INT0 (RB0 / Strobe):** Pro Adress-Strobe wird die Lampen-Adresse und das
  Inhibit-/Datenwort aus `PORTA` gelesen und in `lamp_aux_inhibit[address]`
  abgelegt.
- **Timer2 (One-Shot, Refresh):** 2 ms nach dem Strobe mit Adresse 0 werden die
  gesammelten Zustände in die „echte" Matrix (`lamp[]`) übernommen. Das
  emuliert das Thyristor-Verhalten (Lampe bleibt bis zum nächsten Nulldurchgang
  an; Bally refresht jeden Nulldurchgang, 10 ms @ 50 Hz / 8 ms @ 60 Hz).
- **Hauptschleife / `update_ff()`:** Bei Änderung werden die 8 Flip-Flops
  (74HC574) über `LATC`/`A0..A2` neu beschrieben.
- **IOC (RB6/RB7):** Tasten S1/S2 für den Testmodus.

---

## Befund aus dem Logic-Analyzer-Scan (`lamps.LPF`)

LogicPort-Aufzeichnung, **Sample-Periode 100 ns** (bestätigt über
`AcquiredSamplePeriod` und `CursorOffsetTime/Samples`). Das `.LPF` ist
lauflängencodiert; die Spalte `Count` = Anzahl 100-ns-Samples, die ein Zustand
anhält.

### Kanal-Zuordnung (an `PORTA` des PIC)

| Kanal       | Bedeutung                                            |
|-------------|------------------------------------------------------|
| **D0**      | STROBE (RB0 / INT0), aktiv-HIGH-Puls, ~10 µs breit   |
| **D8–D11**  | Lampen-Adresse (RA0–RA3), 4 Bit binär, D8 = LSB, 15 = Rest |
| **D12–D15** | Inhibit/Daten (RA4–RA7), **0 = Lampe AN**            |

### Gemessenes Timing (ein Adresszyklus, Beispiel Adresse 1)

```
Daten gültig ab        t = 1928.2 µs   (addr=1, data=1111)
Strobe steigt          t = 1943.3 µs   (Daten schon ~15 µs gültig)
Strobe fällt (INT0!)   t = 1953.3 µs   <- Firmware-Trigger (fallende Flanke)
Daten gültig bis       t = 1966.5 µs   (= nur ~13.1 µs nach fallender Flanke)
danach Bus -> Rest:    [addr=1111 | data=0000]
```

Kennzahlen über den gesamten Scan:

- Strobe-Periode (Adresse → Adresse): **~62.6 µs**
- **Datenhaltezeit nach fallender Strobe-Flanke: nur ~13.1 µs** (min/median/max 13.1 / 13.1 / 13.2)
- Daten-Setup **vor** der Flanke: ~25 µs (sehr stabil)
- Voller 15-Adressen-Scan: ~1.5 ms, Wiederholung ~alle 10 ms (→ 50-Hz-Maschine)

---

## Fehlerursache: „fehlerhafte Lampenansteuerung bei einigen Flippern"

Die alte `Int0_Isr()` las die Adresse korrekt an der Flanke, wartete dann aber
`__delay_us(20)` und las **erst danach** die Daten:

```c
address = PORTA & 0x0f;          // korrekt an der Strobe-Flanke
if (address != 15) {
    if (address == 0) T2CONbits.TMR2ON = 1;
    __delay_us(20);              // <-- 20 µs warten
    inhibit = PORTA;             // <-- Bus ist nach ~13 µs schon weitergeschaltet!
    lamp_aux_inhibit[address] = inhibit;
}
```

Da die Daten nur **~13 µs** nach der Flanke stabil sind, liest das 20-µs-Warten
den **nächsten** Buszustand – meist den Rest-Zustand `data=0000`. Wegen
`0 = Lampe AN` werden so Lampen eingeschaltet, die eigentlich AUS sein sollen →
**geisterhafte/falsche Lampen**.

**Warum nur „einige" Flipper?** Das hartkodierte `__delay_us(20)` ist die
fragile Größe. Strobe-Periode und Datenhaltezeit variieren je nach Spiel/MPU,
Netzfrequenz (50/60 Hz) und Takt. Langsamere Maschinen (Haltezeit > 20 µs)
treffen noch ins Fenster → funktionieren; schnellere (62 µs Periode, 13 µs Hold)
nicht. Zusätzlich waren Adresse und Daten **zwei getrennte Reads** (möglicher
Skew).

### Auswertung (Re-Simulation gegen den Scan)

Lesezeitpunkt nach der Strobe-Flanke vs. „an der Flanke gültiger" Wert:

| Read-Zeitpunkt | korrekt gelesene Strobes |
|----------------|--------------------------|
| +0 µs          | **105 / 105**            |
| +3 µs          | **105 / 105** (gewählt)  |
| +5 µs          | **105 / 105**            |
| **+20 µs (alt)** | **0 / 105**            |

---

## Lösung

`Int0_Isr()` liest Adresse **und** Inhibit/Daten in **einem kohärenten
`PORTA`-Read** kurz nach der Strobe-Flanke; nur noch `__delay_us(3)` als
Einschwing-Reserve (≪ 13 µs Haltezeit):

```c
PIR1bits.INT0IF = 0;
__delay_us(3);                 // kurze Einschwing-Reserve, << 13 µs Haltezeit
porta   = PORTA;               // EIN kohärenter Read: low nibble = Adresse, high nibble = Inhibit
address = porta & 0x0f;
if (address != 15) {
    if (address == 0) T2CONbits.TMR2ON = 1;
    lamp_aux_inhibit[address] = porta;
}
```

Korrekt und maschinenunabhängig, weil die Bally-MPU immer **erst Adresse+Daten,
dann Strobe** setzt → die Daten sind an der Flanke garantiert gültig. Ein
einzelner Read macht Adresse und Daten atomar konsistent. Die ISR-Dauer sinkt
von ~20 µs auf ~3 µs (~32 % → < 5 % CPU-Last pro Strobe), was verpasste Strobes
auf schnellen Maschinen zusätzlich vorbeugt.

---

## Bekannte Annahmen / mögliche künftige Härtung

- **Timer2-Refresh-Fenster:** Der Refresh feuert fix 2 ms nach Adresse 0; setzt
  voraus, dass alle 15 Adressen innerhalb dieser 2 ms gestrobt werden (im Scan:
  ~1.5 ms → passt). Bei sehr langsamen/zerstückelten Scans (> 2 ms ab Adr 0)
  könnte der Refresh mitten im Scan feuern (partielles Update/Flackern) →
  ggf. adaptiv lösen.
- **`lamp_aux_inhibit[]`** behält Werte für Adressen, die nicht mehr gestrobt
  werden. Unkritisch, solange jede Adresse pro Scan gestrobt wird (Normalfall).

---

## Verifikation / Test

1. **Build:** Projekt in MPLAB X / VSCode (XC8 v3.10, Device PIC18F25K42) bauen →
   `out/BallyLa_60/default.hex`. (Syntax-Compile mit `xc8-cc.exe` läuft sauber durch.)
2. **Hardware – betroffene (schnelle) Maschine:** Im Ruhezustand/Attract dürfen
   keine Geisterlampen mehr leuchten; einzelne Lampen gezielt schalten und gegen
   das erwartete Spielverhalten prüfen.
3. **Gegentest:** Auf einer bisher funktionierenden (langsamen) und einer bisher
   fehlerhaften (schnellen) Maschine testen, um zu bestätigen, dass die
   flankenbasierte Lesung beide Fälle abdeckt.
