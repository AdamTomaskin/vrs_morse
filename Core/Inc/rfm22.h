#pragma once
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t device_type;
    uint8_t device_version;
} rfm22_info_t;

void rfm22_init(void);
bool rfm22_read_info(rfm22_info_t *out);

bool rfm22_send_packet(const uint8_t *data, uint8_t len, uint32_t timeout_ms);
int  rfm22_poll_receive(uint8_t *out, uint8_t max_len); // returns length, 0=no pkt, -1=crc/error
