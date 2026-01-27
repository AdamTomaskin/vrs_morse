#include "link.h"
#include "rfm22.h"
#include "log.h"

#define LINK_MAGIC0  0xC3
#define LINK_MAGIC1  0x3C

#define LINK_MAX_PKT   32
#define LINK_HDR_LEN   6   // magic0,magic1,msg_id,seq,flags,len
#define LINK_CRC_LEN   1
#define LINK_MAX_PAYLOAD (LINK_MAX_PKT - LINK_HDR_LEN - LINK_CRC_LEN)

#define FLAG_START  (1u<<0)
#define FLAG_END    (1u<<1)

static uint8_t g_msg_id = 0;
static uint8_t g_seq = 0;

static uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

void link_init(void)
{
    rfm22_init();
}

static int link_send_frame(uint8_t msg_id, uint8_t seq, uint8_t flags,
                           const uint8_t *payload, uint8_t len)
{
    uint8_t pkt[LINK_MAX_PKT];
    uint8_t k = 0;

    pkt[k++] = LINK_MAGIC0;
    pkt[k++] = LINK_MAGIC1;
    pkt[k++] = msg_id;
    pkt[k++] = seq;
    pkt[k++] = flags;
    pkt[k++] = len;

    for (uint8_t i = 0; i < len; i++) pkt[k++] = payload[i];

    uint8_t c = crc8(pkt, k);
    pkt[k++] = c;

    // retry 3x
    for (int attempt = 0; attempt < 3; attempt++) {
        if (rfm22_send_packet(pkt, k, 800)) return 1;
    }
    return 0;
}

int link_send_tokens(const uint8_t *toks, uint16_t n)
{
    if (!toks || n == 0) return 0;

    g_msg_id++;

    uint16_t pos = 0;
    while (pos < n) {
        uint8_t chunk = (uint8_t)((n - pos) > LINK_MAX_PAYLOAD ? LINK_MAX_PAYLOAD : (n - pos));
        uint8_t flags = 0;
        if (pos == 0) flags |= FLAG_START;
        if ((pos + chunk) >= n) flags |= FLAG_END;

        uint8_t seq = g_seq++;
        int ok = link_send_frame(g_msg_id, seq, flags, &toks[pos], chunk);
        if (!ok) {
            log_printf("LINK FAIL msg=%u seq=%u\r\n", g_msg_id, seq);
            return 0;
        }
        pos += chunk;
    }

    log_printf("LINK OK msg=%u frames sent\r\n", g_msg_id);
    return 1;
}



