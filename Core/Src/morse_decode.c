#include "morse_decode.h"

// Morse strom (binárny): DOT = ľavá vetva, DASH = pravá vetva.
// Indexovanie: root=1, left=2*i, right=2*i+1
// Naplnené pre A-Z (a pár znakov), čísla riešime samostatne (jednoduchšie).

static char morse_tree_get(const uint8_t *seq, uint8_t len)
{
    // limit: písmená sú max 4 znaky, čísla 5
    // strom tu použijeme pre písmená (len 1..4), čísla riešime mappingom.
    int idx = 1;
    for (uint8_t i = 0; i < len; i++) {
        if (seq[i] == 0x01) idx = 2 * idx;       // DOT
        else if (seq[i] == 0x02) idx = 2 * idx + 1; // DASH
        else return '?';
        if (idx > 63) return '?';
    }

    // tabuľka stromu pre A-Z (indexy do hĺbky 4)
    // index: 1..31 relevantné, zvyšok necháme '?'
    static const char tree[64] = {
        ['\0'] = 0,
        [1] = 0,
        [2] = 'E', [3] = 'T',
        [4] = 'I', [5] = 'A', [6] = 'N', [7] = 'M',
        [8] = 'S', [9] = 'U', [10] = 'R', [11] = 'W',
        [12] = 'D', [13] = 'K', [14] = 'G', [15] = 'O',
        [16] = 'H', [17] = 'V', [18] = 'F', [19] = 0,
        [20] = 'L', [21] = 0,   [22] = 'P', [23] = 'J',
        [24] = 'B', [25] = 'X', [26] = 'C', [27] = 'Y',
        [28] = 'Z', [29] = 'Q', [30] = 0,   [31] = 0
    };

    char c = tree[idx];
    return (c == 0) ? '?' : c;
}

static char decode_digit(const uint8_t *seq, uint8_t len)
{
    // Morse čísla majú vždy 5 znakov:
    // 0 = -----, 1 = .----, 2 = ..---, ... 5 = ....., 6 = -...., ... 9 = ----.
    if (len != 5) return 0;

    // spočítaj úvodné DOT
    int leading_dots = 0;
    while (leading_dots < 5 && seq[leading_dots] == 0x01) leading_dots++;

    // Ak začína DOTmi:
    // 1: .---- (1 dot)
    // 2: ..--- (2 dots)
    // ...
    // 5: ..... (5 dots)
    if (leading_dots > 0) {
        // over zvyšok sú DASH
        for (int i = leading_dots; i < 5; i++) if (seq[i] != 0x02) return 0;
        if (leading_dots == 5) return '5';
        return (char)('0' + leading_dots);
    }

    // Ak začína DASHmi:
    int leading_dashes = 0;
    while (leading_dashes < 5 && seq[leading_dashes] == 0x02) leading_dashes++;
    if (leading_dashes > 0) {
        // zvyšok DOT
        for (int i = leading_dashes; i < 5; i++) if (seq[i] != 0x01) return 0;
        if (leading_dashes == 5) return '0';
        // 6: -.... (1 dash) => 6
        // 7: --... (2 dashes) => 7
        // ...
        // 9: ----. (4 dashes) => 9
        return (char)('5' + leading_dashes);
    }

    return 0;
}

int morse_tokens_to_text(const uint8_t *tokens, uint16_t n_tokens,
                         char *out_text, size_t out_max)
{
    if (!tokens || !out_text || out_max == 0) return MORSE_DECODE_ERR_OVERFLOW;

    size_t out_len = 0;

    uint8_t seq[8];
    uint8_t seq_len = 0;

    for (uint16_t i = 0; i < n_tokens; i++) {
        uint8_t t = tokens[i];

        if (t == 0x01 || t == 0x02) { // DOT/DASH
            if (seq_len < sizeof(seq)) seq[seq_len++] = t;
            else return MORSE_DECODE_ERR_OVERFLOW;
        }
        else if (t == 0x03 || t == 0x04 || t == 0x00) { // END_CHAR / SPACE / END_MSG
            // dokonči znak, ak niečo máme
            if (seq_len > 0) {
                char c = 0;
                // čísla majú 5 znakov, písmená typicky 1..4
                if (seq_len == 5) c = decode_digit(seq, seq_len);
                if (c == 0) c = morse_tree_get(seq, seq_len);

                if (out_len + 1 >= out_max) return MORSE_DECODE_ERR_OVERFLOW;
                out_text[out_len++] = c;
                seq_len = 0;
            }

            // SPACE = medzera medzi slovami
            if (t == 0x04) {
                if (out_len + 1 >= out_max) return MORSE_DECODE_ERR_OVERFLOW;
                out_text[out_len++] = ' ';
            }

            // END_MSG = koniec správy
            if (t == 0x00) break;
        }
        else {
            // neznámy token -> ignoruj alebo označ
        }
    }

    // ak ostalo niečo nedokončené (bez END_CHAR), dokonči
    if (seq_len > 0) {
        char c = 0;
        if (seq_len == 5) c = decode_digit(seq, seq_len);
        if (c == 0) c = morse_tree_get(seq, seq_len);

        if (out_len + 1 >= out_max) return MORSE_DECODE_ERR_OVERFLOW;
        out_text[out_len++] = c;
    }

    out_text[out_len] = '\0';
    return MORSE_DECODE_OK;
}
