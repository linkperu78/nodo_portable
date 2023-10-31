#ifndef __WIFI_ESP32_
#define __WIFI_ESP32_
// ----------------------------------------------------------------- //
#include "lwip/err.h"       // TCP/IP library for ESP32   
#include "lwip/sys.h"       // Functions for time and timing management within the network stack.
#include "credenciales.h"
#include "freertos/FreeRTOS.h"  // It provides a framework for multitasking, task scheduling, and synchronization in embedded applications.
#include "freertos/task.h"      // Header provides functions and macros for creating, starting, and managing tasks
#include "freertos/event_groups.h" // This library is used for creating and managing event groups.

#include "esp_http_client.h"    // API for creating and configuring HTTP/HTTPS clients.
#include "esp_tls.h"        // Transport Layer Security for ESP-IDF - HTTPS
#include "esp_wifi.h"       //  API for connecting to Wi-Fi networks, setting network configurations, and handling events related to Wi-Fi connectivity.
#include "esp_event.h"      //  It allows you to register event handlers for various system and component events, including Wi-Fi
#include "esp_netif.h"      //  Provides an abstraction for network interfaces and allows you to set up and manage network connections

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

/* Define variables for Wifi Connection */
#define DEFAULT_SCAN_LIST_SIZE          5
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1
#define ESP_MAXIMUM_RETRY_CONNECTION    3
#define FAILED_WIFI_SCANNING            "None"

#define cst_wifi_log                    "cst_wifi"

#define my_tag                          "Wifi_API"




void _wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);


void wifi_scan(char* ssid_buffer, size_t buffer_size);


void http_get_data(char* url_path_get, char* response_buffer, size_t size_response_buffer);


int http_post_data(  char* url_path_post, char* data_to_send, size_t data_to_send_size, 
                            char* response_buffer, size_t response_size);


void get_request(esp_http_client_handle_t client, char *response_buffer, size_t buffer_size);


// ----------------------------------------------------------------- //
#endif