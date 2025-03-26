#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "button_monitor.h"

#define BUTTON_GPIO 2
#define BUTTON_LONG_PRESS_TIME_MS 5000

static const char *TAG = "button_monitor";

// Task to monitor the button
static void button_monitor_task(void *pvParameter)
{
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Button monitor task started. Press button on GPIO %d for %d seconds to reset provisioning.", 
            BUTTON_GPIO, BUTTON_LONG_PRESS_TIME_MS / 1000);
    
    TickType_t press_start_time = 0;
    bool button_pressed = false;
    
    while (1) {
        // Read button state (inverted because of pull-up)
        bool current_state = !gpio_get_level(BUTTON_GPIO);
        
        // Button press detected
        if (current_state && !button_pressed) {
            button_pressed = true;
            press_start_time = xTaskGetTickCount();
            ESP_LOGI(TAG, "Button pressed");
        }
        // Button release detected
        else if (!current_state && button_pressed) {
            button_pressed = false;
            ESP_LOGI(TAG, "Button released");
        }
        
        // Check for long press
        if (button_pressed) {
            TickType_t now = xTaskGetTickCount();
            TickType_t press_duration = now - press_start_time;
            
            if (press_duration >= BUTTON_LONG_PRESS_TIME_MS / portTICK_PERIOD_MS) {
                ESP_LOGI(TAG, "Long press detected! Erasing WiFi credentials and restarting...");
                
                // Open NVS and erase WiFi credentials
                nvs_handle_t nvs_handle;
                esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
                if (err == ESP_OK) {
                    // Erase SSID and password
                    nvs_erase_key(nvs_handle, "ssid");
                    nvs_erase_key(nvs_handle, "pass");
                    nvs_commit(nvs_handle);
                    nvs_close(nvs_handle);
                    
                    ESP_LOGI(TAG, "Credentials erased. Restarting...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                } else {
                    ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
                }
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void start_button_monitor(void)
{
    // Create button monitor task
    xTaskCreate(button_monitor_task, "button_monitor_task", 2048, NULL, 5, NULL);
}