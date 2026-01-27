#include "rfm22.h"
#include "rfm22_board.h"
#include "spi.h"
#include "log.h"
#include <stdbool.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;

// ---- RFM22 register addresses (Si4432/RFM22 family) ----
#define REG_DEVICE_TYPE        0x00
#define REG_DEVICE_VERSION     0x01
#define REG_DEVICE_STATUS      0x02
#define REG_INT_STATUS1        0x03
#define REG_INT_STATUS2        0x04
#define REG_INT_ENABLE1        0x05
#define REG_INT_ENABLE2        0x06
#define REG_OP_FUNC_CTRL1      0x07
#define REG_OP_FUNC_CTRL2      0x08

#define REG_PACKET_LEN         0x3E
#define REG_RX_PACKET_LEN      0x4B

#define REG_DATA_ACCESS_CTRL   0x30
#define REG_HEADER_CTRL2       0x33

#define REG_MOD_MODE_CTRL1     0x70
#define REG_MOD_MODE_CTRL2     0x71
#define REG_FREQ_DEV           0x72
#define REG_FREQ_OFF1          0x73
#define REG_FREQ_OFF2          0x74
#define REG_FREQ_BAND_SEL      0x75
#define REG_NOM_CARR_FREQ1     0x76
#define REG_NOM_CARR_FREQ0     0x77

#define REG_TX_POWER           0x6D

#define REG_FIFO_ACCESS        0x7F

// OP_FUNC_CTRL1 bits :contentReference[oaicite:4]{index=4}
#define OPFUNC_SWRESET         0x80
#define OPFUNC_XTON            (1u << 0)
#define OPFUNC_PLLON           (1u << 1)
#define OPFUNC_RXON            (1u << 2)
#define OPFUNC_TXON            (1u << 3)

// IRQ STATUS1 bits :contentReference[oaicite:5]{index=5}
#define IRQ1_ICRCERROR         (1u << 0)
#define IRQ1_IPKVALID          (1u << 1)
#define IRQ1_IPKSENT           (1u << 2)

// OP_FUNC_CTRL2 bits (fifo clear, rxmpk…) :contentReference[oaicite:6]{index=6}
#define OPFUNC2_FFCLRTX        (1u << 0)
#define OPFUNC2_FFCLRRX        (1u << 1)
#define OPFUNC2_RXMPK          (1u << 4)

static inline void cs_low(void)  { HAL_GPIO_WritePin(RFM22_NSEL_GPIO_Port, RFM22_NSEL_Pin, GPIO_PIN_RESET); }
static inline void cs_high(void) { HAL_GPIO_WritePin(RFM22_NSEL_GPIO_Port, RFM22_NSEL_Pin, GPIO_PIN_SET);   }

static uint8_t rfm22_xfer2(uint8_t a, uint8_t d)
{
    uint8_t tx[2] = {a, d};
    uint8_t rx[2] = {0, 0};
    cs_low();
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);
    cs_high();
    return (st == HAL_OK) ? rx[1] : 0xFF;
}

static void rfm22_write_reg(uint8_t reg, uint8_t val)
{
    (void)rfm22_xfer2((uint8_t)(0x80 | (reg & 0x7F)), val);
}

static uint8_t rfm22_read_reg(uint8_t reg)
{
    return rfm22_xfer2((uint8_t)(reg & 0x7F), 0x00);
}

static void rfm22_read_irq(uint8_t *s1, uint8_t *s2)
{
    if (s1) *s1 = rfm22_read_reg(REG_INT_STATUS1);
    if (s2) *s2 = rfm22_read_reg(REG_INT_STATUS2);
}

static void rfm22_fifo_write(const uint8_t *data, uint8_t len)
{
    cs_low();
    uint8_t a = (uint8_t)(0x80 | REG_FIFO_ACCESS);
    HAL_SPI_Transmit(&hspi1, &a, 1, 100);
    HAL_SPI_Transmit(&hspi1, (uint8_t*)data, len, 200);
    cs_high();
}

static void rfm22_fifo_read(uint8_t *data, uint8_t len)
{
    cs_low();
    uint8_t a = (uint8_t)(REG_FIFO_ACCESS & 0x7F);
    HAL_SPI_Transmit(&hspi1, &a, 1, 100);
    HAL_SPI_Receive(&hspi1, data, len, 200);
    cs_high();
}

static void rfm22_fifo_clear_tx(void)
{
    uint8_t r = rfm22_read_reg(REG_OP_FUNC_CTRL2);
    rfm22_write_reg(REG_OP_FUNC_CTRL2, (uint8_t)(r | OPFUNC2_FFCLRTX));
    rfm22_write_reg(REG_OP_FUNC_CTRL2, (uint8_t)(r & (uint8_t)~OPFUNC2_FFCLRTX));
}

static void rfm22_fifo_clear_rx(void)
{
    uint8_t r = rfm22_read_reg(REG_OP_FUNC_CTRL2);
    rfm22_write_reg(REG_OP_FUNC_CTRL2, (uint8_t)(r | OPFUNC2_FFCLRRX));
    rfm22_write_reg(REG_OP_FUNC_CTRL2, (uint8_t)(r & (uint8_t)~OPFUNC2_FFCLRRX));
}

static void rfm22_set_mode_idle(void) { rfm22_write_reg(REG_OP_FUNC_CTRL1, (uint8_t)(OPFUNC_XTON)); }
static void rfm22_set_mode_rx(void)   { rfm22_write_reg(REG_OP_FUNC_CTRL1, (uint8_t)(OPFUNC_XTON | OPFUNC_RXON)); }
static void rfm22_set_mode_tx(void)   { rfm22_write_reg(REG_OP_FUNC_CTRL1, (uint8_t)(OPFUNC_XTON | OPFUNC_TXON)); }

// Nastavenie frekvencie podľa AN440 vzorca (regs 0x75-0x77) :contentReference[oaicite:7]{index=7}
static bool rfm22_set_frequency_khz(uint32_t freq_khz)
{
    uint8_t hbsel = (freq_khz >= 480000u) ? 1u : 0u;
    uint32_t den  = hbsel ? 20000u : 10000u;

    // x64000 = (freq_khz/den - 24) * 64000  (bez floatov)
    uint64_t x64000 = ((uint64_t)freq_khz * 64000u + den/2u) / den;
    if (x64000 < (uint64_t)24u * 64000u) return false;
    x64000 -= (uint64_t)24u * 64000u;

    uint32_t fb = (uint32_t)(x64000 / 64000u);
    uint32_t fc = (uint32_t)(x64000 % 64000u);

    if (fb > 31u) return false;

    // sbsel=1 (recommended), hbsel, fb[4:0]
    uint8_t reg75 = (uint8_t)((1u << 6) | (hbsel << 5) | (fb & 0x1Fu));
    rfm22_write_reg(REG_FREQ_OFF1, 0x00);
    rfm22_write_reg(REG_FREQ_OFF2, 0x00);rfm22_set_mode_idle();
    rfm22_write_reg(REG_FREQ_BAND_SEL, reg75);
    rfm22_write_reg(REG_NOM_CARR_FREQ1, (uint8_t)((fc >> 8) & 0xFF));
    rfm22_write_reg(REG_NOM_CARR_FREQ0, (uint8_t)(fc & 0xFF));

    return true;
}

void rfm22_init(void)
{
#if RFM22_HAS_SDN
    // SDN reset pulse (ak SDN je zapojené)
    HAL_GPIO_WritePin(RFM22_SDN_GPIO_Port, RFM22_SDN_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(RFM22_SDN_GPIO_Port, RFM22_SDN_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
#endif

    // SW reset
    rfm22_write_reg(REG_OP_FUNC_CTRL1, OPFUNC_SWRESET);
    HAL_Delay(20);

    // --- základ: FIFO mode + GFSK ---
    // dtmod=10 (FIFO), modtyp=11 (GFSK) => 0x23 :contentReference[oaicite:8]{index=8}
    rfm22_write_reg(REG_MOD_MODE_CTRL2, 0x23);

    // Packet handler (0x30) – default 0x8D býva OK, ale dávame explicitne
    rfm22_write_reg(REG_DATA_ACCESS_CTRL, 0x8D);

    // IRQ: enable crcerror + pkvalid + pksent (bits 0..2) :contentReference[oaicite:9]{index=9}
    rfm22_write_reg(REG_INT_ENABLE1, (uint8_t)(IRQ1_ICRCERROR | IRQ1_IPKVALID | IRQ1_IPKSENT));
    rfm22_write_reg(REG_INT_ENABLE2, 0x00);

    // RX multi-packet ON (neodíde z RX po prvom packete) :contentReference[oaicite:10]{index=10}
    uint8_t r08 = rfm22_read_reg(REG_OP_FUNC_CTRL2);
    rfm22_write_reg(REG_OP_FUNC_CTRL2, (uint8_t)(r08 | OPFUNC2_RXMPK));

    // frekvencia (ak failne, vypíšeme)
    if (!rfm22_set_frequency_khz(RFM22_DEFAULT_FREQ_KHZ)) {
        log_print("RFM22 FREQ SET FAIL\r\n");
    }

    // TX power (0x6D) – daj zatiaľ konzervatívne 0x18 (default z AN440 tabuľky býva 0x18)
    rfm22_write_reg(REG_TX_POWER, 0x18);

    // clear IRQ (čítanie status regov to typicky clearne)
    uint8_t s1, s2;
    rfm22_read_irq(&s1, &s2);

    // SPI smoke test info
    rfm22_info_t info;
    if (rfm22_read_info(&info)) {
        log_printf("RFM22 INFO: type=0x%02X ver=0x%02X\r\n", info.device_type, info.device_version);
    } else {
        log_print("RFM22 INFO READ FAIL\r\n");
    }

    // --- DEBUG sanity ---
    uint8_t st = rfm22_read_reg(REG_DEVICE_STATUS);
    log_printf("RFM22 STATUS(0x02)=0x%02X\r\n", st);

    rfm22_set_mode_rx();
}

bool rfm22_read_info(rfm22_info_t *out)
{
    if (!out) return false;
    out->device_type = rfm22_read_reg(REG_DEVICE_TYPE);
    out->device_version = rfm22_read_reg(REG_DEVICE_VERSION);
    return true;
}

bool rfm22_send_packet(const uint8_t *data, uint8_t len, uint32_t timeout_ms)
{
    if (!data || len == 0) return false;
    if (len > 64) return false; // FIFO limit

    // clear pending IRQ
    uint8_t s1, s2;
    rfm22_read_irq(&s1, &s2);

    // TX FIFO clear + set packet length (packet handler TX) :contentReference[oaicite:11]{index=11}
    rfm22_fifo_clear_tx();
    rfm22_write_reg(REG_PACKET_LEN, len);

    // write payload
    rfm22_fifo_write(data, len);

    // go TX
    rfm22_set_mode_tx();

    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms) {
        rfm22_read_irq(&s1, &s2);
        if (s1 & IRQ1_IPKSENT) {
            rfm22_set_mode_idle();
            return true;
        }
    }

    rfm22_set_mode_idle();
    return false;
}

int rfm22_poll_receive(uint8_t *out, uint8_t max_len)
{
    if (!out || max_len == 0) return 0;

    uint8_t s1, s2;
    rfm22_read_irq(&s1, &s2);

    if (s1 & IRQ1_ICRCERROR) {
        rfm22_fifo_clear_rx();
        return -1;
    }

    if (s1 & IRQ1_IPKVALID) {
        uint8_t plen = 0;
        rfm22_fifo_read(&plen, 1);     // LEN byte

        if (plen == 0) {
            rfm22_fifo_clear_rx();
            return 0;
        }

        if (plen > max_len) {
            rfm22_fifo_clear_rx();
            return -2;
        }

        rfm22_fifo_read(out, plen);
        rfm22_fifo_clear_rx();
        return (int)plen;
    }

    return 0;
}

