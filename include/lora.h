#ifndef LORA_H
#define LORA_H
#include <stdbool.h>
#include <stdint.h>

void lora_init(void);
bool lora_send_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);
bool lora_join_network(const char *appkey_hex);
bool lora_send_status(const char *msg);

#endif
