#ifndef _ESP_PARSER_H_
#define _ESP_PARSER_H_

#include <ESP8266WiFi.h>
#include "esp_fy6800.h"
#include "esp_config.h"

extern volatile char *gReadBuffer;

void handleWriteMsg(char *msg, uint8_t len);

#endif
