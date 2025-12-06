#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "config.h"
#include "lora.h"
#include "util.h"

static bool read_line(char *buf, size_t max_len, uint32_t timeout_ms) {
    uint32_t start = now_ms();
    size_t idx = 0;
    while (now_ms() - start < timeout_ms) {
        if (uart_is_readable(LORA_UART_ID)) {
            char c = uart_getc(LORA_UART_ID);
            if (c == '\r' || c == '\n') {
                if (idx > 0) { buf[idx] = '\0'; return true; }
            } else if (idx < max_len - 1) {
                buf[idx++] = c;
            }
        }
        tight_loop_contents();
    }
    return false;
}

static void uart_send(const char *s) {
    for (size_t i = 0; i < strlen(s); ++i) uart_putc_raw(LORA_UART_ID, s[i]);
    uart_putc_raw(LORA_UART_ID, '\r');
    uart_putc_raw(LORA_UART_ID, '\n');
}

void lora_init(void) {
    uart_init(LORA_UART_ID, LORA_BAUD);
    gpio_set_function(PIN_LORA_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_LORA_RX, GPIO_FUNC_UART);
    sleep_ms(50);

    // Test with AT
    lora_send_cmd("AT", "OK", LORA_CMD_RESP_TIMEOUT_MS);
}

bool lora_send_cmd(const char *cmd, const char *expect, uint32_t timeout_ms) {
    uart_send(cmd);
    char line[128];
    while (read_line(line, sizeof(line), timeout_ms)) {
        if (strstr(line, expect) != NULL) return true;
        // Keep reading until timeout or expected token
    }
    return false;
}

bool lora_join_network(const char *appkey_hex) {
    char cmd[128];

    if (!lora_send_cmd("AT", "OK", LORA_CMD_RESP_TIMEOUT_MS)) return false;

    snprintf(cmd, sizeof(cmd), "AT+MODE=%s", LORA_MODE);
    if (!lora_send_cmd(cmd, "OK", LORA_CMD_RESP_TIMEOUT_MS)) return false;

    snprintf(cmd, sizeof(cmd), "AT+KEY=APPKEY,\"%s\"", appkey_hex);
    if (!lora_send_cmd(cmd, "OK", LORA_CMD_RESP_TIMEOUT_MS)) return false;

    snprintf(cmd, sizeof(cmd), "AT+CLASS=%c", LORA_CLASS);
    if (!lora_send_cmd(cmd, "OK", LORA_CMD_RESP_TIMEOUT_MS)) return false;

    snprintf(cmd, sizeof(cmd), "AT+PORT=%d", LORA_PORT);
    if (!lora_send_cmd(cmd, "OK", LORA_CMD_RESP_TIMEOUT_MS)) return false;

    // +JOIN can take up to ~20 seconds; retry if fails
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (lora_send_cmd("AT+JOIN", "Joined", LORA_JOIN_TIMEOUT_MS)) {
            return true;
        }
        sleep_ms(1000);
    }
    return false;
}

bool lora_send_status(const char *msg) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MSG=\"%s\"", msg);
    return lora_send_cmd(cmd, "Done", LORA_MSG_TIMEOUT_MS);
}
