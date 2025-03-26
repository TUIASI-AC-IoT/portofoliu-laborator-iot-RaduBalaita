/*  WiFi Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "soft-ap.h"
#include "http-server.h"
#include "normal_mode.h"
#include "button_monitor.h"

#include "../mdns/include/mdns.h"

// Check if WiFi credentials exist in NVS
static bool check_wifi_credentials(void)
{
    static const char *TAG = "check_creds";
    char ssid[32];
    char password[64];
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "SSID not found in NVS");
        nvs_close(nvs_handle);
        return false;
    }
    
    err = nvs_get_str(nvs_handle, "pass", password, &pass_len);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Password not found in NVS");
        nvs_close(nvs_handle);
        return false;
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi credentials found in NVS");
    return true;
}

void app_main(void)
{
    static const char *TAG = "app_main";
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Start the button monitor task
    start_button_monitor();
    
    // Check if WiFi credentials exist
    if (check_wifi_credentials()) {
        // Run normal mode (connect to WiFi and run main app)
        ESP_LOGI(TAG, "WiFi credentials found - starting normal mode");
        run_normal_mode();
    } else {
        // Run provisioning mode
        ESP_LOGI(TAG, "No WiFi credentials found - starting provisioning mode");
        
        // Start SoftAP
        ESP_LOGI(TAG, "Starting SoftAP mode");
        wifi_init_softap();

        // Scan for WiFi networks
        ESP_LOGI(TAG, "Starting WiFi scanning");
        wifi_scan_start();

        // Conditionally initialize mDNS if enabled
#if CONFIG_MDNS_ENABLE
        ESP_LOGI(TAG, "Initializing mDNS");
        ESP_ERROR_CHECK(mdns_init());
        ESP_ERROR_CHECK(mdns_hostname_set("setup"));
        ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Setup Portal"));
        ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
        ESP_LOGI(TAG, "mDNS hostname set to: setup.local");
#endif

        // Start HTTP server for provisioning
        ESP_LOGI(TAG, "Starting HTTP server");
        start_webserver();
        
        ESP_LOGI(TAG, "Provisioning system ready!");
    }
}