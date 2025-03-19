/*  WiFi softAP Example

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

#include "../mdns/include/mdns.h"

void app_main(void)
{
    static const char *TAG = "app_main";
    
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // TODO: 1. Start the softAP mode
    ESP_LOGI(TAG, "Starting softAP mode");
    wifi_init_softap();

    // TODO: 3. SSID scanning in STA mode 
    ESP_LOGI(TAG, "Starting WiFi scanning");
    wifi_scan_start();

    // TODO: 4. mDNS init (if there is time left)
    ESP_LOGI(TAG, "Initializing mDNS");
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("setup"));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Setup Portal"));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_LOGI(TAG, "mDNS hostname set to: setup.local");

    // TODO: 2. Start the web server
    ESP_LOGI(TAG, "Starting HTTP server");
    start_webserver();
    
    ESP_LOGI(TAG, "Provisioning system ready!");
}