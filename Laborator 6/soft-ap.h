#ifndef SOFT_AP_H
#define SOFT_AP_H

#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"

#define EXAMPLE_ESP_WIFI_SSID      "ESP32-prov-<nume-familie>"
#define EXAMPLE_ESP_WIFI_PASS      "password"
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN       4

#define MAX_AP_NUM 20

// SoftAP configuration
void wifi_init_softap(void);

// WiFi scanning
void wifi_scan_start(void);

// Scanned AP records
extern wifi_ap_record_t ap_records[MAX_AP_NUM];
extern uint16_t ap_count;

#endif /* SOFT_AP_H */