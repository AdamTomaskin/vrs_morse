#ifndef BUTTON_H
#define BUTTON_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_low;

    uint8_t raw_level;
    uint8_t stable_level;

    uint32_t last_raw_change_ms;
    uint32_t last_stable_change_ms;

    uint32_t debounce_ms;

    bool pressed_event;
    bool released_event;
} button_t;

void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin, bool active_low, uint32_t debounce_ms);
void button_update(button_t *b, uint32_t now_ms);

bool button_was_pressed(button_t *b);
bool button_was_released(button_t *b);
bool button_is_down(const button_t *b);

uint32_t button_last_stable_change_ms(const button_t *b);

#endif
