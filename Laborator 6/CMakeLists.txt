# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS "main.c" "http-server.c" "soft-ap.c" "button_monitor.c" "normal_mode.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_http_server esp_wifi mdns
)
