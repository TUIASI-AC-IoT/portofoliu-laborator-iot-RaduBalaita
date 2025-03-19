#include <string.h>
#include <sys/param.h>
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

#include "esp_http_server.h"
#include "http-server.h"

static const char *TAG = "http-server";
static httpd_handle_t server = NULL;

// Create the index page HTML
static char* generate_index_html(void) {
    static char html_response[4096];
    memset(html_response, 0, sizeof(html_response));
    
    // Start with HTML header
    strcpy(html_response, "<html><body><form action=\"/results.html\" method=\"post\">");
    strcat(html_response, "<label for=\"fname\">Networks found:</label><br>");
    strcat(html_response, "<select name=\"ssid\">");
    
    // Add each scanned network to the dropdown
    for (int i = 0; i < ap_count; i++) {
        char option[128];
        snprintf(option, sizeof(option), 
                "<option value=\"%s\">%s</option>", 
                (char*)ap_records[i].ssid, 
                (char*)ap_records[i].ssid);
        strcat(html_response, option);
    }
    
    // Complete the form
    strcat(html_response, "</select><br>");
    strcat(html_response, "<label for=\"ipass\">Security key:</label><br>");
    strcat(html_response, "<input type=\"password\" name=\"ipass\"><br>");
    strcat(html_response, "<input type=\"submit\" value=\"Submit\">");
    strcat(html_response, "</form></body></html>");
    
    return html_response;
}

// Handler for GET request at root path "/"
static esp_err_t index_get_handler(httpd_req_t *req)
{
    char* html = generate_index_html();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

// Handler for POST request at "/results.html"
static esp_err_t results_post_handler(httpd_req_t *req)
{
    // Increase buffer size to avoid truncation
    char content[512];
    char ssid[32] = {0};
    char password[64] = {0};
    
    // Read POST data
    int ret, remaining = req->content_len;
    char buf[200] = {0};
    
    if (remaining > sizeof(buf) - 1) {
        // Data too long to handle
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    while (remaining > 0) {
        // Read data from the request
        ret = httpd_req_recv(req, buf + strlen(buf), remaining);
        if (ret <= 0) {
            // Error or timeout, return immediately
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    // buf now contains "ssid=SSID_NAME&ipass=PASSWORD"
    ESP_LOGI(TAG, "Received form data: %s", buf);
    
    // Parse SSID value
    char *ssid_start = strstr(buf, "ssid=");
    if (ssid_start) {
        ssid_start += 5; // Move past "ssid="
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end) {
            strncpy(ssid, ssid_start, ssid_end - ssid_start);
            ssid[ssid_end - ssid_start] = '\0';
        }
    }
    
    // Parse password value
    char *pass_start = strstr(buf, "ipass=");
    if (pass_start) {
        pass_start += 6; // Move past "ipass="
        strcpy(password, pass_start);
    }
    
    ESP_LOGI(TAG, "Parsed SSID: %s, Password: %s", ssid, password);
    
    // Store WiFi credentials in NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_set_str(nvs_handle, "wifi_ssid", ssid);
        nvs_set_str(nvs_handle, "wifi_pass", password);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "WiFi credentials stored in NVS");
    } else {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
    }
    
    // Create response by building it in parts to avoid format truncation
    strcpy(content, "<html><body>");
    strcat(content, "<h1>WiFi Configuration Saved</h1>");
    
    // Add SSID safely
    strcat(content, "<p>SSID: ");
    strncat(content, ssid, sizeof(content) - strlen(content) - 1);
    strcat(content, "</p>");
    
    // Add password safely
    strcat(content, "<p>Password: ");
    strncat(content, password, sizeof(content) - strlen(content) - 1);
    strcat(content, "</p>");
    
    strcat(content, "</body></html>");
    
    // Send response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, content, strlen(content));
    
    return ESP_OK;
}

// URI handlers
static const httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t results_uri = {
    .uri       = "/results.html",
    .method    = HTTP_POST,
    .handler   = results_post_handler,
    .user_ctx  = NULL
};

// Start the web server
void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &results_uri);
    } else {
        ESP_LOGI(TAG, "Error starting server!");
    }
}