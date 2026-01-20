# VRS – Morse encoder/decoder

Semestrálny projekt z predmetu **VRS (Vnorené riadiace systémy)**.  
Cieľom je preniesť správu zadanú v Morseovej abecede z jednej STM32 dosky (TX) na druhú (RX) cez **RFM22 Wireless shield** a na prijímači ju **dekódovať** a zobraziť na **TFT 240xRGBx320** displeji.

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
