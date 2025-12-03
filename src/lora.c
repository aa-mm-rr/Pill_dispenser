#include "lora.h"
#include "board.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <string.h>
#include <stdio.h>

static bool uart_read_until(char* out, int maxlen, const char* want, uint32_t timeout_ms){
  int n = 0;
  absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
  while (absolute_time_diff_us(get_absolute_time(), deadline) < 0 && n < maxlen-1){
    if (uart_is_readable(LORA_UART)){
      char c = uart_getc(LORA_UART);
      out[n++] = c;
      out[n] = 0;
      if (strstr(out, want)) return true;
    } else {
      sleep_ms(5);
    }
  }
  return false;
}

static void uart_putsln(const char* s){
  uart_puts(LORA_UART, s);
  uart_puts(LORA_UART, "\r\n");
}

void lora_init(void){
  uart_init(LORA_UART, LORA_BAUD);
  gpio_set_function(PIN_LORA_TX, GPIO_FUNC_UART);
  gpio_set_function(PIN_LORA_RX, GPIO_FUNC_UART);
  sleep_ms(50);
  uart_putsln("AT");
  char buf[256]={0};
  uart_read_until(buf, sizeof(buf), "+AT: OK", 500);
  uart_putsln("AT+MODE=LWOTAA");
  uart_read_until(buf, sizeof(buf), "+MODE: LWOTAA", 1000);
  lora_set_class_a();
  lora_set_port(8);
}

void lora_set_port(uint8_t port){
  char cmd[32]; snprintf(cmd,sizeof(cmd),"AT+PORT=%u",port);
  uart_putsln(cmd); char r[128]={0};
  uart_read_until(r,sizeof(r), "+PORT:", 1000);
}

void lora_set_class_a(void){
  uart_putsln("AT+CLASS=A"); char r[128]={0};
  uart_read_until(r,sizeof(r), "+CLASS: A", 1000);
}

bool lora_join_otaa(const char* appkey_hex){
  char r[256]={0};
  // AppEui can stay default (8000000000000006) on your module or set explicitly if required
  // AppKey must be set
  char cmd[96];
  snprintf(cmd,sizeof(cmd),"AT+KEY=APPKEY,\"%s\"",appkey_hex);
  uart_putsln(cmd); uart_read_until(r,sizeof(r), "+KEY: APPKEY", 1000);

  uart_putsln("AT+JOIN");
  // JOIN can take up to ~20s
  if (uart_read_until(r,sizeof(r), "+JOIN: Done", 20000)) return true;
  return false;
}

bool lora_send_msg(const char* text, uint32_t timeout_ms){
  char cmd[160]; snprintf(cmd,sizeof(cmd),"AT+MSG=\"%s\"", text);
  uart_putsln(cmd);
  char r[256]={0};
  return uart_read_until(r, sizeof(r), "+MSG: Done", timeout_ms);
}

void lora_report(const char* tag){
  // Helper to send short status lines
  lora_send_msg(tag, 8000);
}
