#ifndef MORSE_INPUT_H
#define MORSE_INPUT_H

#include "button.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MORSE_END_MSG  = 0x00,
    MORSE_DOT      = 0x01,
    MORSE_DASH     = 0x02,
    MORSE_END_CHAR = 0x03,
    MORSE_SPACE    = 0x04
} morse_token_t;

typedef struct {
    button_t *btn;

    uint32_t T_ms;

    bool in_press;
    uint32_t press_start_ms;
    uint32_t last_release_ms;

    uint8_t buf[256];
    uint16_t len;

    bool message_ready;
} morse_input_t;

void morse_input_init(morse_input_t *m, button_t *btn, uint32_t T_ms);
void morse_input_set_T(morse_input_t *m, uint32_t T_ms);

void morse_input_task(morse_input_t *m, uint32_t now_ms);

bool morse_input_message_ready(const morse_input_t *m);
uint16_t morse_input_take_message(morse_input_t *m, uint8_t *out, uint16_t max);

#endif
