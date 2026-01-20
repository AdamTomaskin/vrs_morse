#include "morse_input.h"

static void push_token(morse_input_t *m, uint8_t tok)
{
    if (m->len < sizeof(m->buf)) {
        m->buf[m->len++] = tok;
    }
}

static uint8_t last_token(const morse_input_t *m)
{
    if (m->len == 0) return 0xFF;
    return m->buf[m->len - 1];
}

void morse_input_init(morse_input_t *m, button_t *btn, uint32_t T_ms)
{
    m->btn = btn;
    m->T_ms = T_ms;

    m->in_press = false;
    m->press_start_ms = 0;
    m->last_release_ms = HAL_GetTick();

    m->len = 0;
    m->message_ready = false;
}

void morse_input_set_T(morse_input_t *m, uint32_t T_ms)
{
    m->T_ms = T_ms;
}

void morse_input_task(morse_input_t *m, uint32_t now_ms)
{
    // 1) press/release -> DOT/DASH
    if (button_was_pressed(m->btn)) {
        m->in_press = true;
        m->press_start_ms = now_ms;
    }

    if (button_was_released(m->btn)) {
        m->in_press = false;
        uint32_t dur = now_ms - m->press_start_ms;

        // prah: < 2T = DOT, >= 2T = DASH
        if (dur < (2u * m->T_ms)) push_token(m, MORSE_DOT);
        else                      push_token(m, MORSE_DASH);

        m->last_release_ms = now_ms;
    }

    // 2) gaps -> END_CHAR / SPACE / END_MSG
    if (!m->in_press && m->len > 0 && !m->message_ready) {
        uint32_t gap = now_ms - m->last_release_ms;

        // SPACE po 7T
        if (gap >= (7u * m->T_ms)) {
            uint8_t lt = last_token(m);
            if (lt != MORSE_SPACE && lt != MORSE_END_MSG) {
                push_token(m, MORSE_SPACE);
            }
        }
        // END_CHAR po 3T (ale nie ak už je SPACE)
        else if (gap >= (3u * m->T_ms)) {
            uint8_t lt = last_token(m);
            if (lt != MORSE_END_CHAR && lt != MORSE_SPACE && lt != MORSE_END_MSG) {
                push_token(m, MORSE_END_CHAR);
            }
        }

        // END_MSG po 10T (ukončenie správy)
        if (gap >= (10u * m->T_ms)) {
            uint8_t lt = last_token(m);
            if (lt != MORSE_END_MSG) {
                push_token(m, MORSE_END_MSG);
            }
            m->message_ready = true;
        }
    }
}

bool morse_input_message_ready(const morse_input_t *m)
{
    return m->message_ready;
}

uint16_t morse_input_take_message(morse_input_t *m, uint8_t *out, uint16_t max)
{
    if (!m->message_ready) return 0;

    uint16_t n = (m->len < max) ? m->len : max;
    for (uint16_t i = 0; i < n; i++) out[i] = m->buf[i];

    // reset
    m->len = 0;
    m->message_ready = false;
    m->last_release_ms = HAL_GetTick();
    return n;
}
