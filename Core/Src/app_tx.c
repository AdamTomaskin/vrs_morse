#include "app_tx.h"
#include "log.h"
#include "button.h"
#include "morse_input.h"
#include <string.h>
#include "morse_decode.h"
#include "link.h"  // ADD
#include "rfm22.h"
#ifndef BTN_GPIO_Port
#define BTN_GPIO_Port GPIOB
#endif
#ifndef BTN_Pin
#define BTN_Pin GPIO_PIN_1
#endif

extern UART_HandleTypeDef huart2;

static button_t g_btn;
static morse_input_t g_morse;

static void tokens_to_ascii(const uint8_t *toks, uint16_t n, char *out, uint16_t out_max)
{
    uint16_t k = 0;
    for (uint16_t i = 0; i < n && k + 1 < out_max; i++) {
        switch (toks[i]) {
            case MORSE_DOT:      out[k++] = '.'; break;
            case MORSE_DASH:     out[k++] = '-'; break;
            case MORSE_END_CHAR: out[k++] = ' '; break;
            case MORSE_SPACE:    out[k++] = '/'; break;
            case MORSE_END_MSG:  /* stop */ i = n; break;
            default: break;
        }
    }
    while (k > 0 && (out[k-1] == ' ' || out[k-1] == '/')) {
           k--;
       }
    out[k] = '\0';
}

void app_tx_init(void)
{
    log_init(&huart2);

    // PB1 + Pull-Up => active-low tlačidlo (stlačené = 0)
    button_init(&g_btn, BTN_GPIO_Port, BTN_Pin, true, 20); // 20 ms debounce
    morse_input_init(&g_morse, &g_btn, 150);               // T = 150 ms

    log_print("TX READY\r\n");
    log_print("BTN: active-low (pressed=0)\r\n");
}

void app_tx_task(void)
{
    uint32_t now = HAL_GetTick();

    // DEBUG: každé 2 sekundy pošli dlhý token stream cez LINK (test chunkingu)
    static uint32_t last = 0;
    if (now - last >= 2000) {
        last = now;

        uint8_t toks[200];
        uint16_t n = 0;

        // vyrob dlhú "správu" z tokenov (len na test)
        // pattern: .-.-.- ... s občasným END_CHAR a nakoniec END_MSG
        for (int i = 0; i < 160 && n < (sizeof(toks) - 2); i++) {
            toks[n++] = (i & 1) ? MORSE_DOT : MORSE_DASH;

            // každých 6 znakov ukonči "písmeno"
            if ((i % 6) == 5 && n < (sizeof(toks) - 2)) {
                toks[n++] = MORSE_END_CHAR;
            }

            // každých ~30 znakov sprav "medzeru medzi slovami"
            if ((i % 30) == 29 && n < (sizeof(toks) - 2)) {
                toks[n++] = MORSE_SPACE;
            }
        }

        toks[n++] = MORSE_END_MSG;

        int ok = link_send_tokens(toks, n);
        log_printf("DBG LINK TOKENS: %s (n=%u)\r\n", ok ? "OK" : "FAIL", (unsigned)n);
    }
    // ---- koniec debugu ----

    button_update(&g_btn, now);
    morse_input_task(&g_morse, now);

    if (morse_input_message_ready(&g_morse)) {
        uint8_t toks[256];
        uint16_t n = morse_input_take_message(&g_morse, toks, sizeof(toks));

        char line[128];
        tokens_to_ascii(toks, n, line, sizeof(line));

        char text[64];
        morse_tokens_to_text(toks, n, text, sizeof(text));

        log_printf("MSG TOKENS=%u  MORSE= %s  TEXT= %s\r\n", (unsigned)n, line, text);

        int ok = link_send_tokens(toks, n);
        log_printf("LINK SEND: %s\r\n", ok ? "OK" : "FAIL");
    }

}
