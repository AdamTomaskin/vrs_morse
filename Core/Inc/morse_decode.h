#ifndef MORSE_DECODE_H
#define MORSE_DECODE_H

#include <stdint.h>
#include <stddef.h>

#define MORSE_DECODE_OK          0
#define MORSE_DECODE_ERR_UNKNOWN 1
#define MORSE_DECODE_ERR_OVERFLOW 2

// tokeny: 0x01 DOT, 0x02 DASH, 0x03 END_CHAR, 0x04 SPACE, 0x00 END_MSG
int morse_tokens_to_text(const uint8_t *tokens, uint16_t n_tokens,
                         char *out_text, size_t out_max);

#endif
