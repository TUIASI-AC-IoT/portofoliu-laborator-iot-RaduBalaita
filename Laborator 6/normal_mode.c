#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Changed from mdns.h to esp_mdns.h for built-in mDNS support
#if CONFIG_MDNS_ENABLE
#include "mdns.h"
#endif

#include "normal_mode.h"

static const char *TAG = "normal_mode";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAXIMUM_RETRY 5

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Connect to the AP failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(const char* ssid, const char* password)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    // Copy SSID and password to config
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid) + 1);
    memcpy(wifi_config.sta.password, password, strlen(password) + 1);

    ESP_LOGI(TAG, "Connecting to %s...", ssid);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", ssid);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// mDNS service browsing function - conditionally compiled
#if CONFIG_MDNS_ENABLE
static void mdns_browse_services(void)
{
    ESP_LOGI(TAG, "Browsing for mDNS services...");
    
    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    
    // Browse for HTTP services
    ESP_LOGI(TAG, "Browsing for _http._tcp services...");
    mdns_result_t *results = NULL;
    esp_err_t err = mdns_query_ptr("_http", "_tcp", 3000, 20, &results);
    if (err) {
        ESP_LOGE(TAG, "mDNS query failed: %s", esp_err_to_name(err));
        return;
    }
    
    if (!results) {
        ESP_LOGI(TAG, "No mDNS services found!");
        return;
    }
    
    mdns_result_t *r = results;
    while (r) {
        ESP_LOGI(TAG, "Service: %s.%s.local", r->instance_name, r->service_type);
        
        // Print IP addresses
        mdns_ip_addr_t *addr = r->addr;
        while (addr) {
            char ip_str[16];
            if (addr->addr.type == ESP_IPADDR_TYPE_V4) {
                sprintf(ip_str, IPSTR, IP2STR(&addr->addr.u_addr.ip4));
                ESP_LOGI(TAG, "  IPv4: %s", ip_str);
            } else if (addr->addr.type == ESP_IPADDR_TYPE_V6) {
                ESP_LOGI(TAG, "  IPv6: (not displayed)");
            }
            addr = addr->next;
        }
        
        // Print TXT records
        if (r->txt_count > 0) {
            ESP_LOGI(TAG, "  TXT records:");
            for (int i = 0; i < r->txt_count; i++) {
                ESP_LOGI(TAG, "    %s: %s", r->txt[i].key, r->txt[i].value);
            }
        }
        
        // Print service port
        ESP_LOGI(TAG, "  Port: %d", r->port);
        
        r = r->next;
    }
    
    // Free the results
    mdns_query_results_free(results);
}
#endif

void run_normal_mode(void)
{
    char ssid[32];
    char password[64];
    
    // Get WiFi credentials from NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return;
    }
    
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);
    
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading SSID from NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }
    
    err = nvs_get_str(nvs_handle, "pass", password, &pass_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading password from NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }
    
    nvs_close(nvs_handle);
    
    // Connect to WiFi
    ESP_LOGI(TAG, "Starting normal mode operation");
    wifi_init_sta(ssid, password);
    
    // Browse for mDNS services - conditionally compiled
#if CONFIG_MDNS_ENABLE
    mdns_browse_services();
#else
    ESP_LOGI(TAG, "mDNS disabled in configuration");
#endif
}