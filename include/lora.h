#pragma once
#include <stdbool.h>
#include <stdint.h>

void lora_init(void);
bool lora_join_otaa(const char* appkey_hex);
bool lora_send_msg(const char* text, uint32_t timeout_ms);
void lora_set_port(uint8_t port);
void lora_set_class_a(void);
void lora_report(const char* tag);
