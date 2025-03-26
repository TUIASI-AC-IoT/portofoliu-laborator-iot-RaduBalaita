#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "soft-ap.h"

// Start the HTTP web server
void start_webserver(void);

#endif /* HTTP_SERVER_H */