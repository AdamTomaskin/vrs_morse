# VRS – Morse encoder/decoder

Semestrálny projekt z predmetu **VRS (Vnorené riadiace systémy)**.  
Cieľom je preniesť správu zadanú v Morseovej abecede z jednej STM32 dosky (TX) na druhú (RX) cez **RFM22 Wireless shield** a na prijímači ju **dekódovať** a zobraziť na **TFT 240xRGBx320** displeji.

---

## Použité súčiastky
- 2× STM32 doska (TX + RX)
- 2× RFM22 Wireless (TX + RX)
- 1× TFT displej 240×320 (RGB) – na RX strane
- 1× tlačidlo – na TX strane
- breadboard + vodiče + napájanie 3.3 V (spoločná GND)

---

## Členovia tímu
- Adam Tomaškin
- Jakub Gajdoš
- Šimon Chrobák

---

## Čo projekt robí

### TX (vysielač)
- Používateľ zadáva Morseovu správu **tlačidlom** (DOT/DASH podľa dĺžky stlačenia, medzery podľa dĺžky pauzy).
- STM32 spraví z vstupu **Morse tokeny** a odošle ich cez **RFM22 Wireless shield**.

### RX (prijímač)
- Druhá STM32 prijme tokeny cez **RFM22 Wireless shield**.
- Tokeny **dekóduje** na text (písmená/čísla) a zároveň vie zobraziť aj samotnú morzeovku (`.` `-`).
- Výsledok zobrazí na **TFT 240xRGBx320** (voliteľne aj stav linky: OK/CRC ERR/LOST).

---

## Rozdelenie úloh

### 1) Adam Tomáškin – Morse vstup + TX aplikácia
- Debounce tlačidla, meranie dĺžky stlačenia a pauzy
- Generovanie tokenov DOT/DASH/END_CHAR/SPACE/END_MSG
- TX state machine a príprava dát pre odoslanie
- Debug výpisy tokenov a dekódovaného textu cez UART

### 2) Jakub Gajdoš – Bezdrôtová komunikácia (RFM22)
- ovládanie RFM22 a jeho napojenie na STM32
- odosielanie a prijímanie dát medzi TX a RX
- jednoduché overenie správnosti (napr. CRC/počítadlo paketov) + testovanie linky

### 3) Šimon Chrobák – TFT displej + UI (RX)
- ovládanie TFT 240×320 a vykresľovanie textu
- zobrazenie prijatej morzeovky a dekódovaného textu
- používateľské rozhranie (stav prenosu, posledná správa, prípadne posúvanie)
