#pragma once
#include <stdint.h>
#include <stdbool.h>

void link_init(void);

// vráti 1=OK, 0=fail (po retry)
int link_send_tokens(const uint8_t *toks, uint16_t n);

