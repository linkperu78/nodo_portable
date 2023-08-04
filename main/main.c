/*

 * Author: Diego Quispe
 * Contact: diegoar.quispec@gmail.com 
 * Date: Tuesday July 25, 2023
 * Description: "Nodo Portable" CST Group's Project
 *              This code if for Wifi Connection - HTTP Request
 *              and SD read/write operations
 * Warnings:    This code contains comments in spanish and 
 *              english languages
*/

#include "credenciales.h"
#include "esp32_general.h"
#include "esp32_sd.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#include "esp_http_client.h"


/* Define variables for Wifi Connection */
#define DEFAULT_SCAN_LIST_SIZE          5
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1
#define ESP_MAXIMUM_RETRY_CONNECTION    3
#define FAILED_WIFI_SCANNING            "None"

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

typedef struct 
{   const char* ssid;
    const char* password;
} StructAP;

StructAP myListAP[] = {
    {EDGE_AP, EDGE_PASS},
    {MODEM_AP, MODEM_PASS},
};

/* Define variable size for HTTP Request */
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 20480


/* Define variables for logs */
static const char *TAG = "Nodo_Portable";


                /*  --  FOR WIFI CONNECTIONS --  */
/* This handler is just for get connection and IP value  */
static void _wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        //esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY_CONNECTION) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Ip asignada = " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Initialize Wi-Fi as STA and set scan method */
static void wifi_scan(char* ssid_buffer, size_t buffer_size)
{
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_LOGI(TAG, " - Preconfiguramos el Wifi\n");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    /* Configure the RX and TX buffers for Wi-Fi communication */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*  Setemoas las banderas a usar para verificar la conexion hacia el 
        Access Point */
    ESP_LOGI(TAG, " - Creamos los hanlders segun evento\n");    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &_wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    /* Set the maximum number of APs to scan */
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_LOGI(TAG, " - Iniciamos el ESP32 Wifi Module\n");
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, " - Escaneamos redes cercanas\n");

    /* Scan for APs in the surroundings */
    esp_wifi_scan_start(NULL, true);

    /* Get the scanned AP records */
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    /* Get the number of scanned APs */
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    //ESP_LOGI(TAG, "\t TOTAL APs scanned = %u\n", ap_count);
    int ap_index;
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        int ap_found = 0;
        for(ap_index = 0; ap_index < sizeof(myListAP) / sizeof(StructAP); ap_index++){
            // Buscamos entre la lista de los AP, los AP deseados ESP-AP y WIFILOCAL
            if ( strcmp( (const char*)ap_info[i].ssid, myListAP[ap_index].ssid ) != 0 ){
                continue;
                }
            ap_found = 1;
            break;
        }
        if(ap_found != 0){
            break;
        }
        ap_index = -1;
    }

    /* Si no hay AP de la lista, no nos conectamos y nos vamos a dormir*/
    if (ap_index < 0){
        ESP_LOGE(TAG, " No se encontraron redes cercanas\n");
        strncpy(ssid_buffer, FAILED_WIFI_SCANNING, buffer_size - 1);
        ssid_buffer[buffer_size - 1] = '\0';
        return;
    }

    /* Si hay una AP, nos conectamos */
    // Copiamos el nombre del AP encontrado
    char ssid_password[20];
    size_t ssidpassword_len = sizeof(ssid_password);
    // SSID name copy
    strncpy(ssid_buffer, (const char*)myListAP[ap_index].ssid, buffer_size - 1);
    ssid_buffer[buffer_size - 1] = '\0';
    // Password copy
    strncpy(ssid_password, (const char*)myListAP[ap_index].password, ssidpassword_len -1 );
    ssid_password[ssidpassword_len - 1] = '\0';

    // Configuramos los parametros para conectarnos al AP
    ESP_LOGI(TAG, "Reconfiguramos para pasar de scaner a point to point\n");
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid, ssid_buffer, buffer_size);
    memcpy(wifi_config.sta.password, ssid_password, ssidpassword_len);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    /* Iniciamos la conexion hacia el Wi-Fi Access Point deseado */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Iniciamos el intento de conexion \n");

    /* Seteamos los datos del hanlder para esperar a que nos conectemos al Wi-Fi deseado*/
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to:\n\tSSID: %s\n\tPassword: %s\n",
                 ssid_buffer, ssid_password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to:\n\tSSID: %s\n\tPassword: %s\n",
                 ssid_buffer, ssid_password);
        strncpy(ssid_buffer, FAILED_WIFI_SCANNING, buffer_size - 1);
        ssid_buffer[buffer_size - 1] = '\0';
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        strncpy(ssid_buffer, FAILED_WIFI_SCANNING, buffer_size - 1);
        ssid_buffer[buffer_size - 1] = '\0';
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}


                /*  --  FOR HTTP REQUEST --  */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        default:
            ESP_LOGI(TAG,"ANOTHER ISSUES \n");
            break;
    }
    return ESP_OK;
}

/*              GET() REQUEST              */
static void http_get_data(char* url_path_get, char* local_response_buffer){
    int content_length = 0;
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    esp_http_client_config_t config = {
        .url = url_path_get,
        .timeout_ms = 5000,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return;
    }

    content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
        esp_http_client_close(client);
        return;
    }

    if (content_length >= MAX_HTTP_OUTPUT_BUFFER) {
        ESP_LOGE(TAG, "Buffer size insufficent:\n - Needed = %d\n - Available = %d\n", content_length, MAX_HTTP_OUTPUT_BUFFER);
        esp_http_client_close(client);
        return;
    }

    int data_read = esp_http_client_read_response(client, local_response_buffer, MAX_HTTP_OUTPUT_BUFFER);
    if (data_read < 0 ){
        ESP_LOGE(TAG, "Failed to read response");
        esp_http_client_close(client);
        return;
    }

    int get_response = esp_http_client_get_status_code(client);
    
    if(get_response != 200){
        ESP_LOGE(TAG, "Fallo en el request GET(), HTTP Status = %d\n", get_response);
        esp_http_client_close(client);
        return;
    }

    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));

    esp_http_client_close(client);
    return;
}


/*              POST() REQUEST              */
static int http_post_data(char* url_path_post, char* data_to_send, size_t data_to_send_size, char* response_buffer, size_t response_size) {
    ESP_LOGI(TAG, "POST Request to:\n - %s\n", url_path_post);
    int client_length_response = 0;
    memset(response_buffer, 0, response_size);

    esp_http_client_config_t config = {
        .url                = url_path_post,
        .timeout_ms         = 10000,
        .event_handler      = _http_event_handler,
        .crt_bundle_attach  = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    // Set Content-Type header
    esp_http_client_set_header(client, "Content-Type", tpi_format);
    // Set ApiKey header
    esp_http_client_set_header(client, "ApiKey", tpi_key);

    esp_err_t err_post = esp_http_client_open(client, data_to_send_size);
    if (err_post != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err_post));
        return -1;
    }

    client_length_response = esp_http_client_write(client, data_to_send, data_to_send_size);
    client_length_response += esp_http_client_fetch_headers(client);
    
    if (client_length_response < 0) {
        ESP_LOGE(TAG, "Failed writting data in endpoint\n");
        esp_http_client_cleanup(client);
        return -1;
    }

    client_length_response = esp_http_client_read_response(client, response_buffer, response_size);
    if(client_length_response < 0){
        esp_http_client_cleanup(client);
        return -1;
    }
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG, "HTTP POST Status = %d\nRespuesta:\n%s\n", status_code, response_buffer);
    if(status_code != 200){
        return -1;
    }

    return 1;
}


/*
    -------------------------- PROGRAMA PRINCIPAL --------------------------
*/
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    /* Desahibilitamos ciertos logs */
    esp_log_level_set("wifi_init", ESP_LOG_WARN);
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("phy_init", ESP_LOG_WARN);    
    
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Inicializamos el programa prinicpal\n");

    /* Intentamos conectarnos a un ACCESS POINT y mostramos el IP obtenido */
    char ssid_buffer[32];
    wifi_scan(ssid_buffer, sizeof(ssid_buffer));
    printf("Best SSID: %s\n", ssid_buffer);

    if( strcmp(FAILED_WIFI_SCANNING , ssid_buffer) == 0){
        ESP_LOGE(TAG, "Finalizamos por no poder conectarse a una red Wifi \n");
        sleep_ESP32(TIME_TO_SLEEP);
    }

    /* Configuramos los pines de la ESP32 */
    activate_pin(PinSD);
    config_pin();

    /* Creamos el buffer para HTTP Request */
    static char esp_http_buffer[MAX_HTTP_OUTPUT_BUFFER];
    //static size_t buffer_size = sizeof(esp_http_buffer);

    #define len_file_name  30
    char file_name[len_file_name] = "";
    char url_to_get[100] = "";
    int number_packages = 0;

    /* Montamos la tarjeta SD para su uso */
    static sdmmc_card_t *card = NULL;
    sdmmc_host_t host;
    esp_err_t ret_SD = init_SD(&card, &host);

    if (ret_SD != ESP_OK) {
        ESP_LOGE(TAG, "TARJETA SD NO DETECTADA\n, se va a apagar el equipo\n");
        return;
    }

    // Una vez conectados, elegimos que operacion hacer segun el nombre del AP
     /*              WIFI = ESP-AP (EDGE_AP)         */
    if ( strcmp(EDGE_AP, ssid_buffer) == 0 ){
        printf(" \t FUNCION: Empezamos la extraccion de Datos \n");

        /* Datos de Salud */
        sprintf(url_to_get, "http://%s%s", edge_server, edge_salud_size); 
        ESP_LOGI(TAG, "El servidor de size = %s\n", url_to_get);

        // Obtenemos el numero de paquetes
        http_get_data(url_to_get, esp_http_buffer);

        // Lo almacenamos en un .txt file como cabecera
        sprintf(file_name, "%s.txt", file_salud_size);
        guardar_file_sd(esp_http_buffer, file_name);

        int intValue = atoi(esp_http_buffer);

        sprintf(url_to_get, "http://%s%s", edge_server, edge_salud_data); 
        ESP_LOGI(TAG, "El servidor de datos = %s\n", url_to_get);

        for (int iteration = 0; iteration < intValue; iteration++){
            http_get_data(url_to_get, esp_http_buffer);
            sprintf(file_name, "%s%d.txt", file_salud_data, iteration);
            guardar_file_sd(esp_http_buffer, file_name);
        }

        /* Datos de Pesaje */

    }
    /*              WIFI = WIFILOCAL  (MODEM_AP)         */
    else if ( strcmp(MODEM_AP, ssid_buffer) == 0 ){
        esp_log_level_set("HTTP_CLIENT", ESP_LOG_DEBUG);

        /* Leemos la cantidad de archivos a leer de SALUD */
        sprintf(file_name, "%s.txt", file_salud_size);
        leer_file_sd(file_name, esp_http_buffer, MAX_HTTP_OUTPUT_BUFFER);
        number_packages = atoi(esp_http_buffer);
        ESP_LOGI(TAG, "Archivo SALUD: Se enviaran %d paquetes de datos", number_packages);

        #define http_post_size  100
        char http_post_response[http_post_size];
        
        //number_packages = 1;
        int result_post = 0;
        
        for(int iterator = 0; iterator < number_packages; iterator++){
            sprintf(file_name, "%s%d.txt", file_salud_data, iterator);
            result_post = 0;
            // Obtenemos el los datos almacenados en la SD
            size_t length_sd = leer_file_sd(file_name, esp_http_buffer, MAX_HTTP_OUTPUT_BUFFER);
            if (length_sd == 0){
                ESP_LOGE(TAG, "No hay data compatible con endpoint\n");
                continue;
            }

            // CST Group endpoint POST() Request
            sprintf(url_to_get, "%s%s", cst_server, cst_salud); 
            result_post += http_post_data(url_to_get, esp_http_buffer, length_sd, http_post_response, http_post_size);
            ESP_LOGE(TAG, "Data result = %d\n", result_post);

            // OMNICLOUD endpoint POST() Request
            sprintf(url_to_get, "%s%s", tpi_server, tpi_salud);
            result_post += http_post_data(url_to_get, esp_http_buffer, length_sd, http_post_response, http_post_size);
            ESP_LOGE(TAG, "Data result = %d\n", result_post);
            if(result_post == 2){
                ESP_LOGI(TAG, "\t ------- Eliminando archivo ... \n");
                //delete_file_sd(file_name);
            }
        }
    }

    // Desmomtamos la tarjeta SD ya que nos vamos a dormir
    ESP_LOGI(TAG, " - Ejectamos la tarjeta SD\n");
    deactivate_pin(PinSD);
    eject_SD(card, &host);

    ESP_LOGI(TAG, " - Apagamos las luces LED \n");
    power_off_leds();
    ESP_LOGI(TAG, " - Apagamos el Modulo WIFI \n");
    ESP_ERROR_CHECK( esp_wifi_stop() );

    //ESP_LOGI(TAG, "[APP] Free memory: %lu bytes", esp_get_free_heap_size());

    ESP_LOGI(TAG, " \n\t Comenzamos el deep_sleep \n");
    // Dormimos el equipo por 10 minutos
    sleep_ESP32(TIME_TO_SLEEP);
}
