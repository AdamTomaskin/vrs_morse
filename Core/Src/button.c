#include "button.h"

static uint8_t read_level(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1u : 0u;
}

void button_init(button_t *b, GPIO_TypeDef *port, uint16_t pin, bool active_low, uint32_t debounce_ms)
{
    b->port = port;
    b->pin = pin;
    b->active_low = active_low;
    b->debounce_ms = debounce_ms;

    uint32_t now = HAL_GetTick();
    b->raw_level = read_level(port, pin);
    b->stable_level = b->raw_level;
    b->last_raw_change_ms = now;
    b->last_stable_change_ms = now;

    b->pressed_event = false;
    b->released_event = false;
}

void button_update(button_t *b, uint32_t now_ms)
{
    b->pressed_event = false;
    b->released_event = false;

    uint8_t lvl = read_level(b->port, b->pin);

    if (lvl != b->raw_level) {
        b->raw_level = lvl;
        b->last_raw_change_ms = now_ms;
    }

    if (b->raw_level != b->stable_level) {
        if ((now_ms - b->last_raw_change_ms) >= b->debounce_ms) {
            b->stable_level = b->raw_level;
            b->last_stable_change_ms = now_ms;

            bool down = button_is_down(b);
            if (down) b->pressed_event = true;
            else      b->released_event = true;
        }
    }
}

bool button_was_pressed(button_t *b)
{
    bool v = b->pressed_event;
    b->pressed_event = false;
    return v;
}

bool button_was_released(button_t *b)
{
    bool v = b->released_event;
    b->released_event = false;
    return v;
}

bool button_is_down(const button_t *b)
{
    if (b->active_low) {
        return (b->stable_level == 0u);
    } else {
        return (b->stable_level == 1u);
    }
}

uint32_t button_last_stable_change_ms(const button_t *b)
{
    return b->last_stable_change_ms;
}
