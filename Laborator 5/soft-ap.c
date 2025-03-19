#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/event_groups.h"

#include "soft-ap.h"

#define WIFI_SOFT_AP_STARTED_BIT BIT0
#define MAX_AP_NUM 20

EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "soft-ap";

// Define the AP records array and count
wifi_ap_record_t ap_records[MAX_AP_NUM];
uint16_t ap_count = 0;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan completed");
        
        // Get number of APs found
        uint16_t number = MAX_AP_NUM;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&number));
        ap_count = (number > MAX_AP_NUM) ? MAX_AP_NUM : number;
        
        // Get AP records
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
        
        // Print scan results
        ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI(TAG, "SSID \"%s\", RSSI %d", ap_records[i].ssid, ap_records[i].rssi);
        }
    } else if (event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Event: SoftAP started");
        xEventGroupSetBits(s_wifi_event_group, WIFI_SOFT_AP_STARTED_BIT);
    }
}

void wifi_init_softap(void)
{
    s_wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default netif instance for SoftAP
    esp_netif_create_default_wifi_ap();
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                       ESP_EVENT_ANY_ID,
                                                       &wifi_event_handler,
                                                       NULL,
                                                       NULL));
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Configure SoftAP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "esp32-familyname", // Replace with your family name
            .ssid_len = 0, // Use strlen
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 1
        },
    };
    
    // Set WiFi mode to APSTA (both AP and STA interfaces active)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Set SoftAP configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "SoftAP started with SSID: %s, password: %s", 
             wifi_config.ap.ssid, wifi_config.ap.password);
}

void wifi_scan_start(void)
{
    // Make sure WiFi is in APSTA mode
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    if (mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }
    
    // Configure scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,        // Scan all channels
        .show_hidden = false, // Don't show hidden SSIDs
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };
    
    // Start scan
    ESP_LOGI(TAG, "Starting WiFi scan...");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_LOGI(TAG, "WiFi scan completed");
}
