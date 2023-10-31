#include "esp32_wifi.h"
#include "esp32_general.h"

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


/* This handler is just for get connection and IP value  */
void _wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        //esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY_CONNECTION) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(my_tag, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(my_tag,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(my_tag, "Ip asignada = " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/* Initialize Wi-Fi as STA and set scan method */
void wifi_scan(char* ssid_buffer, size_t buffer_size)
{
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_LOGI(my_tag, " - Preconfiguramos el Wifi\n");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    //assert(sta_netif);

    /* Configure the RX and TX buffers for Wi-Fi communication */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*  Setemoas las banderas a usar para verificar la conexion hacia el 
        Access Point */
    ESP_LOGI(my_tag, " - Creamos los hanlders segun evento\n");    
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

    ESP_LOGI(my_tag, " - Iniciamos el ESP32 Wifi Module\n");
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(my_tag, " - Escaneamos redes cercanas\n");

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
        ESP_LOGE(my_tag, " No se encontraron redes cercanas\n");
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
    ssid_buffer[buffer_size - 1] = '\0';    // add a null-terminated for robust

    // Password copy
    strncpy(ssid_password, (const char*)myListAP[ap_index].password, ssidpassword_len -1 );
    ssid_password[ssidpassword_len - 1] = '\0';


    // Configuramos los parametros para conectarnos al AP
    ESP_LOGI(my_tag, "Reconfiguramos para pasar de scaner a point to point\n");
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid, ssid_buffer, buffer_size);
    memcpy(wifi_config.sta.password, ssid_password, ssidpassword_len);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    /* Iniciamos la conexion hacia el Wi-Fi Access Point deseado */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(my_tag, "Iniciamos el intento de conexion \n");

    /* Seteamos los datos del hanlder para esperar a que nos conectemos al Wi-Fi deseado*/
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(my_tag, "Connected to:\n\tSSID: %s\n\tPassword: %s\n",
                 ssid_buffer, ssid_password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(my_tag, "Failed to connect to:\n\tSSID: %s\n\tPassword: %s\n",
                 ssid_buffer, ssid_password);
        strncpy(ssid_buffer, FAILED_WIFI_SCANNING, buffer_size - 1);
        ssid_buffer[buffer_size - 1] = '\0';
    } else {
        ESP_LOGE(my_tag, "UNEXPECTED EVENT");
        strncpy(ssid_buffer, FAILED_WIFI_SCANNING, buffer_size - 1);
        ssid_buffer[buffer_size - 1] = '\0';
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}


/*              GET() REQUEST              */
void http_get_data(char* url_path_get, char* response_buffer, size_t size_response_buffer){
    int content_length = 0;
    memset(response_buffer, 0, size_response_buffer);

    ESP_LOGI(my_tag, "Nos conectamos a la URL : '%s'\n", url_path_get);

    esp_http_client_config_t config = {
        .url = url_path_get,
        .timeout_ms = 5000,
        //.event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);

    if (err != ESP_OK) {
        ESP_LOGE(my_tag, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        led_set(WIFI, YELLOW);
        return;
    }

    content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(my_tag, "HTTP client fetch headers failed");
        esp_http_client_close(client);
        led_set(WIFI, YELLOW);
        return;
    }

    if (content_length >= size_response_buffer) {
        ESP_LOGE(my_tag, "Buffer size insufficent:\n - Needed = %d\n - Available = %d\n", content_length, size_response_buffer);
        esp_http_client_close(client);
        led_set(WIFI, RED);
        return;
    }

    int data_read = esp_http_client_read_response(client, response_buffer, size_response_buffer);
    if (data_read < 0 ){
        ESP_LOGE(my_tag, "Failed to read response");
        esp_http_client_close(client);
        led_set(WIFI, RED);
        return;
    }

    int get_response = esp_http_client_get_status_code(client);
    
    if(get_response != 200){
        ESP_LOGE(my_tag, "Fallo en el request GET(), HTTP Status = %d\n", get_response);
        esp_http_client_close(client);
        led_set(WIFI, RED);
        return;
    }

    ESP_LOGI(my_tag, "HTTP GET Status = %d, content_length = %"PRId64,
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));

    esp_http_client_close(client);
    led_set(WIFI, GREEN);
}


/*              POST() REQUEST              */
int http_post_data(  char* url_path_post, char* data_to_send, size_t data_to_send_size, 
                            char* response_buffer, size_t response_size ) {
    ESP_LOGI(my_tag, "POST Request to:\n**%s\n", url_path_post);
    int client_length_response = 0;
    memset(response_buffer, 0, response_size);

    esp_http_client_config_t config = {
        .url                = url_path_post,
        .timeout_ms         = 10000,
        //.event_handler      = _http_event_handler,
        .crt_bundle_attach  = esp_crt_bundle_attach,
    };
    ESP_LOGI(my_tag, "Clear the response buffer 101\n");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    // Set Content-Type header
    esp_http_client_set_header(client, "Content-Type", tpi_format);
    // Set ApiKey header
    esp_http_client_set_header(client, "ApiKey", tpi_key);

    esp_err_t err_post = esp_http_client_open(client, data_to_send_size);
    if (err_post != ESP_OK) {
        ESP_LOGE(my_tag, "Failed to open HTTP connection: %s", esp_err_to_name(err_post));
        led_set(WIFI, RED);
        return -1;
    }
    client_length_response = esp_http_client_write(client, data_to_send, data_to_send_size);
    client_length_response += esp_http_client_fetch_headers(client);

    if (client_length_response < 0) {
        ESP_LOGE(my_tag, "Failed writting data in endpoint\n");
        esp_http_client_cleanup(client);
        led_set(WIFI, RED);
        return -1;
    }

    client_length_response = esp_http_client_read_response(client, response_buffer, response_size);
    if(client_length_response < 0){
        esp_http_client_cleanup(client);
        led_set(WIFI, RED);
        return -1;
    }
    ESP_LOGI(my_tag, "Clear the response buffer 2\n");
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    
    ESP_LOGI(my_tag, "HTTP POST Status = %d\nRespuesta:\n%s\n", status_code, response_buffer);
    if(status_code != 200){
        led_set(WIFI, RED);
        return -1;
    }
    led_set(WIFI, BLUE);
    return 1;
}


void get_request(esp_http_client_handle_t client, char *response_buffer, size_t buffer_size) {
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    memset(response_buffer, 0, buffer_size);
    esp_err_t esp_http_err = esp_http_client_open(client, 0);

    if (esp_http_err == ESP_OK) {
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(my_tag, "HTTP client fetch headers failed");
        } else {
            int data_read = esp_http_client_read_response(client, response_buffer, buffer_size);
            if (data_read >= 0) {
                ESP_LOGI(my_tag, "- Server respondio = %d", esp_http_client_get_status_code(client));
            } else {
                ESP_LOGE(my_tag, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
}

