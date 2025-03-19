#include "../mdns/include/mdns.h"
#include "esp_log.h"

static const char *TAG = "mdns_stub";

esp_err_t mdns_init(void) {
    ESP_LOGW(TAG, "Using stub mDNS implementation");
    return ESP_OK;
}

esp_err_t mdns_hostname_set(const char *hostname) {
    ESP_LOGW(TAG, "Setting stub hostname: %s", hostname);
    return ESP_OK;
}

esp_err_t mdns_instance_name_set(const char *instance_name) {
    ESP_LOGW(TAG, "Setting stub instance name: %s", instance_name);
    return ESP_OK;
}

esp_err_t mdns_service_add(const char *instance_name, const char *service_type,
                          const char *proto, uint16_t port,
                          mdns_txt_item_t *txt_items, size_t num_items) {
    ESP_LOGW(TAG, "Adding stub service: %s.%s.%s on port %d",
             instance_name ? instance_name : "(default)", service_type, proto, port);
    return ESP_OK;
}