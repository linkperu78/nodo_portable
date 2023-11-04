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
#include "esp32_wifi.h"

#include <sys/param.h>

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif


/* Define variable size for HTTP Request */
#define MAX_HTTP_OUTPUT_BUFFER  20480       // For read package data from SQL Database
#define http_post_size          200         // For response from server after a POST() Request
#define sd_file_buffer           30          // For storage name of file (max len_file_name)


/* Define variables for logs */
static const char *TAG = "Nodo_Portable";   // For Debug message title
float battery_value = 0.00;

void update_led_battery(){
    if (battery_value > 4.0){
        led_set(BAT, BLUE);
    }
    else{
        if ( battery_value > 3.75 ) {
            led_set(BAT, GREEN);
        }
        else {
            led_set(BAT, RED);
        }
    }
}

esp_err_t update_battery_file(char* localtime, float battery_level) {
    // Nombre del archivo donde se va a guardar el nuevo dato
    char new_file_name[30];
    sprintf(new_file_name, "/sdcard/bateria.txt");
    char battery_buffer[150];

    // Le damos el formato para almacenarlo en bateria.txt
    sprintf(battery_buffer, "{\"Valor\":\"%.2f\",\"Identificador\":\"Voltaje\",\"Fecha\":\"%s\"}",
            battery_level, localtime);
    ESP_LOGI("SD_CARD", "New batterry entry = %s\n", battery_buffer);

    // Evaluamos la cantidad de datos en el contenido
    if(file_exists(new_file_name)){
        // File exists
        FILE* f = fopen(new_file_name, "r+");
        ESP_LOGI("SD_CARD", "Ingresando nuevo valor\n");
        fseek(f, -2, SEEK_END);
        ftruncate(fileno(f), ftell(f));
        fprintf(f,",");
        fprintf(f, "%s", battery_buffer);
        fprintf(f,"]}");
        fclose(f);
    }
    else{
        uint8_t base_mac[6];
        esp_wifi_get_mac(ESP_IF_WIFI_STA, base_mac);
        char macId[20];
        uint8_t index = 0;
        for(uint8_t i=0; i<6; i++){
            index += sprintf(&macId[index], "%02x", base_mac[i]);
        }
        int id_empresa = 1;
        char cargadora[9] = "EQP44";
        char prefix_battery_file[250];
        sprintf(prefix_battery_file, "{\"idEmpresa\":%d,\"idDispositivo\":\"%s\",\"Cargadora\":\"%s\",\"registro\":[%s]}", 
                id_empresa, macId, cargadora, battery_buffer);
        ESP_LOGE("SD_CARD", "NEW FILE = %s\n", prefix_battery_file);
        FILE* f = fopen(new_file_name, "a");
        fprintf(f, "%s", prefix_battery_file);
        fclose(f);
    }
    return ESP_OK;
}


void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    config_pin();
    ADC_Channel_configure(BAT_ADC_CHANNEL);

    /* Desahibilitamos ciertos logs */
    esp_log_level_set("wifi_init", ESP_LOG_WARN);
    esp_log_level_set("wifi", ESP_LOG_WARN);
    //esp_log_level_set("phy_init", ESP_LOG_WARN);    
    

    // -----------------  Datos de Bateria ----------------------
    battery_value = adc_get_value(BAT_ADC_CHANNEL);
    ESP_LOGI(TAG, "Valor leido de bateria = %.2f\n", battery_value);
    update_led_battery();
    

    ESP_LOGI(TAG, "Inicializamos el programa prinicpal\n");

    /* Intentamos conectarnos a un ACCESS POINT y mostramos el IP obtenido */
    led_set(WIFI, WHITE);  
    char ssid_buffer[32];

    wifi_scan(ssid_buffer, sizeof(ssid_buffer));

    printf("\t\t - - - - Mejor red encontrada es '%s' - - - -\n\n", ssid_buffer);

    if( strcmp(FAILED_WIFI_SCANNING , ssid_buffer) == 0){
        ESP_LOGE(TAG, "Finalizamos por no poder conectarse a una red Wifi \n");
        led_set(WIFI, RED);
        delay_ms(500);
        sleep_ESP32(TIME_TO_SLEEP);
    }

    // Encendemos el LED segun el resultado obtenido
    if ( strcmp(EDGE_AP, ssid_buffer) == 0 ){
        led_set(WIFI, GREEN);
    }
    else if ( strcmp(MODEM_AP, ssid_buffer) == 0 ){
        led_set(WIFI, BLUE);
    }


    /* Configuramos los pines de la ESP32 */
    activate_pin(PinSD);

    /* Creamos el buffer para HTTP Request */
    static char buffer_http_response[MAX_HTTP_OUTPUT_BUFFER];
    static char buffer_file_name[sd_file_buffer];
    static char buffer_sd_qty[sd_file_buffer];
    char buffer_url[100] = "";
    size_t free_heap_size = 0;

    int old_qty_salud = 0;
    int new_qty_salud = 0;

    /* Montamos la tarjeta SD para su uso */
    static sdmmc_card_t *card = NULL;
    sdmmc_host_t host;

    esp_err_t ret_SD = init_SD(&card, &host);

    led_set(CHECK, WHITE);
    if (ret_SD != ESP_OK) {
        ESP_LOGE(TAG, "TARJETA SD NO DETECTADA\n, se va a apagar el equipo\n");
        led_set(CHECK, RED);
        delay_ms(1000);
        sleep_ESP32(TIME_TO_SLEEP);
    }

    // Inspeccionamos si existen los archivos
    // salud nuevo
    sprintf(buffer_file_name, "%s.txt", file_salud_size);
    if (file_exists(buffer_file_name) == 0 ){
        led_set(CHECK, YELLOW);
        create_file(buffer_file_name, "0");
        led_set(CHECK, OFF);
    }
    
    // pesaje nuevo
    sprintf(buffer_file_name, "%s.txt", file_pesaje_size);
    if (file_exists(buffer_file_name) == 0 ){
        led_set(CHECK, YELLOW);
        create_file(buffer_file_name, "0");
        led_set(CHECK, OFF);
    }
    
    // salud error
    sprintf(buffer_file_name, "%s.txt", file_err_salud_size);
    if (file_exists(buffer_file_name) == 0 ){
        led_set(CHECK, YELLOW);
        create_file(buffer_file_name, "0");
        led_set(CHECK, OFF);
    }
    led_set(CHECK, GREEN);

    // Creamos un cliente para hacer HTTP Request
    // *Observacion*: Si se crea varios clientes la memoria disponible se reduce
    // *Recomendacion*: Usar un mismo cliente y reconfigurarlo varias veces, finalizar con WiFi
    esp_err_t esp_http_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = cst_server,
        .timeout_ms = 5000,
        // Opcional: Agregar un handler para chequear mejor la interaccion
        //.event_handler = _http_event_handler,
    };
    config.event_handler = NULL;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_cleanup(client);


    // ---------------------------------------------------
    //              WIFI = ESP-AP (EDGE_AP)
    // ---------------------------------------------------
    if ( strcmp(EDGE_AP, ssid_buffer) == 0 ){
        printf(" \n\t\t - - - - Empezamos la extraccion de Datos - - - - \n");
        
        // Set URL : La hora local
        char localtime_buffer[60];
        sprintf(buffer_url, "http://%s%s", edge_server, edge_localtime);
        config.url = buffer_url;
        client = esp_http_client_init(&config);
        get_request(client, localtime_buffer, sizeof(localtime_buffer));
        esp_http_client_cleanup(client);
        printf(" Data obtenida = '%s' - '%d'\n", localtime_buffer, sizeof(localtime_buffer));

        // ----------------- Datos de Salud ---------------------- 
        // Apuntamos al servidor del Edge Computer que contiene la cantidad
        // de paquetes almacenados
        sprintf(buffer_url, "http://%s%s", edge_server, edge_salud_size);
        config.url = buffer_url;
        client = esp_http_client_init(&config);
        get_request(client, buffer_sd_qty, sizeof(buffer_sd_qty));
        esp_http_client_cleanup(client);
        new_qty_salud = atoi(buffer_sd_qty);

        // Leemos los archivos ya almacenados en Salud
        sprintf(buffer_file_name,"%s.txt", file_salud_size);
        leer_file_sd(buffer_file_name, buffer_sd_qty, sizeof(buffer_sd_qty));
        old_qty_salud = atoi(buffer_sd_qty);

        //ESP_LOGI(TAG, " - Antiguos archivos = %d \n\t\t -Nuevos archivos = %d\n", old_qty_salud, new_qty_salud);
        
        int new_total = old_qty_salud + new_qty_salud;
        // Guardamos el nuevo valor de datos totales
        sprintf(buffer_file_name,"%s.txt", file_salud_size);
        sprintf(buffer_sd_qty, "%d", new_total);
        guardar_file_sd(buffer_sd_qty, buffer_file_name);

        ESP_LOGI(TAG, "Cantidad de nuevos datos = '%d'\n", new_qty_salud);

        // Apuntamos el cliente http hacia la URL de datos de Salud
        sprintf(buffer_url, "http://%s%s", edge_server, edge_salud_data);
        config.url = buffer_url;
        config.timeout_ms = 5000;
        client = esp_http_client_init(&config);
        for (int n = old_qty_salud; n < new_total; n++){
            // Solicitamos datos al url = "/datos"
            //free_heap_size = esp_get_free_heap_size();
            //ESP_LOGI(TAG, "Free space = '%u'\n", free_heap_size);
            // Obtenemos la nueva data
            get_request(client, buffer_http_response, sizeof(buffer_http_response));
            // Creamos el archivo sa_number.txt
            sprintf(buffer_file_name,"%s%d.txt", file_salud_data, n);
            create_file(buffer_file_name, buffer_http_response);
            delay_ms(100);
        }
        esp_http_client_cleanup(client);
    }
    

    // ---------------------------------------------------
    //              WIFI = WIFILOCAL (MODEM_AP)
    // ---------------------------------------------------
    if ( strcmp(MODEM_AP, ssid_buffer) == 0 ){
        printf(" \n\t\t - - - - Empezamos el envio de Datos - - - - \n");
        int qty_files = 0;
        int qty_err_files = 0;
        int actual_failed_files = 0;
        int resultado = 0;
        int check_sd_length = 0;

        // --- Salud Datos fallidos
        sprintf(buffer_file_name, "%s.txt", file_err_salud_size);
        leer_file_sd(buffer_file_name, buffer_sd_qty, sizeof(buffer_sd_qty));
        // Obtenemos la cantidad de datos fallidos
        qty_err_files = atoi(buffer_sd_qty);
        actual_failed_files = 0;

        // --- Salud Nuevos datos        
        sprintf(buffer_file_name,"%s.txt", file_salud_size);
        leer_file_sd(buffer_file_name, buffer_sd_qty, sizeof(buffer_sd_qty));
        // Obtenemos la cantidad de datos nuevos
        qty_files = atoi(buffer_sd_qty);

        //  ------------------ CST SERVER ---------------------------
        // Iniciamos el cliente
        sprintf(buffer_url, "%s%s", cst_server, cst_salud);
        ESP_LOGI(TAG, "\n \t ------- Enviando datos a : '%s'  -------\n", buffer_url);
        config.url = buffer_url;
        config.timeout_ms = 10000;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.disable_auto_redirect = true;
        client = esp_http_client_init(&config);
        // Set HTTP Method
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        // Set Content-Type header
        esp_http_client_set_header(client, "Content-Type", tpi_format);
        // Set ApiKey header
        esp_http_client_set_header(client, "ApiKey", tpi_key);

        for(int n = 0; n < qty_err_files; n++){
            // Leemos el archivo a enviar
            sprintf(buffer_file_name,"%s%d.txt", file_err_salud_dat, n);
            if (file_exists(buffer_file_name) == 0){
                continue;
            }
            memset(buffer_http_response, 0, sizeof(buffer_http_response));
            check_sd_length = leer_file_sd(buffer_file_name, buffer_http_response, sizeof(buffer_http_response));
            if (check_sd_length < 1){
                delete_file_sd(buffer_file_name);
                continue;
            }

            // Intentamos enviar datos hacia el servidor
            // Si ocurre un error, el archivo se mantiene y no es eliminado
            esp_http_client_set_post_field(client, buffer_http_response, strlen(buffer_http_response));
            ret = esp_http_client_perform(client);
            resultado = esp_http_client_get_status_code(client);
            if(ret == ESP_OK && resultado == 200){
                ESP_LOGI(TAG, " \t- El archivo %s se envio correctamente\n", buffer_file_name);
            }
            else{
                ESP_LOGE(TAG, " \t- Fallo al enviar el archivo %s\n", buffer_file_name);
            }
        }

        for(int n = 0; n < qty_files; n++){
            // Leemos el archivo a enviar
            sprintf(buffer_file_name,"%s%d.txt", file_salud_data, n);
            if (file_exists(buffer_file_name) == 0){
                continue;
            }
            memset(buffer_http_response, 0, sizeof(buffer_http_response));
            check_sd_length = leer_file_sd(buffer_file_name, buffer_http_response, sizeof(buffer_http_response));
            if (check_sd_length < 1){
                delete_file_sd(buffer_file_name);
                continue;
            }
            // Intentamos enviar datos hacia el servidor
            // Si ocurre un error, el archivo se mantiene y no es eliminado
            esp_http_client_set_post_field(client, buffer_http_response, strlen(buffer_http_response));
            ret = esp_http_client_perform(client);
            resultado = esp_http_client_get_status_code(client);
            if(ret == ESP_OK && resultado == 200){
                ESP_LOGI(TAG, " \t- El archivo %s se envio correctamente\n", buffer_file_name);
            }
            else{
                ESP_LOGE(TAG, " \t- Fallo al enviar el archivo %s\n", buffer_file_name);
            }
        }
        esp_http_client_cleanup(client);

        //  ------------------ TPI SERVER ---------------------------
        // Iniciamos el cliente
        sprintf(buffer_url, "%s%s", tpi_server, tpi_salud);
        ESP_LOGI(TAG, "\n \t ------- Enviando datos a : '%s'  -------\n", buffer_url);
        config.url = buffer_url;
        config.timeout_ms = 10000;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.disable_auto_redirect = true;
        client = esp_http_client_init(&config);
        // Set HTTP Method
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        // Set Content-Type header
        esp_http_client_set_header(client, "Content-Type", tpi_format);
        // Set ApiKey header
        esp_http_client_set_header(client, "ApiKey", tpi_key);

        for(int n = 0; n < qty_err_files; n++){
            // Leemos el archivo a enviar
            sprintf(buffer_file_name,"%s%d.txt", file_err_salud_dat, n);
            if (file_exists(buffer_file_name) == 0){
                continue;
            }
            memset(buffer_http_response, 0, sizeof(buffer_http_response));
            leer_file_sd(buffer_file_name, buffer_http_response, sizeof(buffer_http_response));

            // Intentamos enviar datos hacia el servidor
            // Si ocurre un error, el archivo se mantiene y no es eliminado
            esp_http_client_set_post_field(client, buffer_http_response, strlen(buffer_http_response));
            ret = esp_http_client_perform(client);
            resultado = esp_http_client_get_status_code(client);
            if(ret == ESP_OK && resultado == 200){
                ESP_LOGI(TAG, " \t- El archivo %s se envio correctamente\n", buffer_file_name);
                delete_file_sd(buffer_file_name);
            }
            else{
                ESP_LOGE(TAG, " \t- Fallo al enviar el archivo %s\n", buffer_file_name);
                sprintf(buffer_file_name, "%s%d.txt", file_err_salud_dat, actual_failed_files);
                ESP_LOGI(TAG, " \t\t- Creando el archivo %s\n", buffer_file_name);
                create_file(buffer_file_name, buffer_http_response);
                actual_failed_files++;
            }
        }

        for(int n = 0; n < qty_files; n++){
            // Leemos el archivo a enviar
            sprintf(buffer_file_name,"%s%d.txt", file_salud_data, n);
            if (file_exists(buffer_file_name) == 0){
                continue;
            }
            memset(buffer_http_response, 0, sizeof(buffer_http_response));
            leer_file_sd(buffer_file_name, buffer_http_response, sizeof(buffer_http_response));

            // Intentamos enviar datos hacia el servidor
            // Si ocurre un error, el archivo se mantiene y no es eliminado
            esp_http_client_set_post_field(client, buffer_http_response, strlen(buffer_http_response));
            ret = esp_http_client_perform(client);
            resultado = esp_http_client_get_status_code(client);
            if(ret == ESP_OK && resultado == 200){
                ESP_LOGI(TAG, " \t- El archivo %s se envio correctamente\n", buffer_file_name);
                delete_file_sd(buffer_file_name);
            }
            else{
                ESP_LOGE(TAG, " \t- Fallo al enviar el archivo %s\n", buffer_file_name);
                sprintf(buffer_file_name, "%s%d.txt", file_err_salud_dat, actual_failed_files);
                ESP_LOGI(TAG, " \t\t- Creando el archivo %s\n", buffer_file_name);
                create_file(buffer_file_name, buffer_http_response);
                actual_failed_files++;
            }
        }
        esp_http_client_cleanup(client);
        
        sprintf(buffer_file_name, "%s.txt", file_salud_size);
        delete_file_sd(buffer_file_name);
        delay_ms(30);
        create_file(buffer_file_name, "0");
        
        sprintf(buffer_file_name, "%s.txt", file_err_salud_size);
        delete_file_sd(buffer_file_name);
        delay_ms(30);
        char new_size_err[10];
        sprintf(new_size_err, "%d", actual_failed_files);
        create_file(buffer_file_name, new_size_err);
    }

    // --------------  END PROGRAM  ----------------
    led_set(CHECK, GREEN);    
    ESP_LOGI(TAG, " - Apagamos el Modulo WIFI \n");
    ESP_ERROR_CHECK( esp_wifi_stop() );
    led_set(WIFI, WHITE);

    ESP_LOGI(TAG, " - Ejectamos la tarjeta SD\n");
    eject_SD(card, &host);
    deactivate_pin(PinSD);

    ESP_LOGI(TAG, " - Apagamos las luces LED \n");
    power_off_leds();
    delay_ms(500);

    ESP_LOGI(TAG, " \n\t Comenzamos el deep_sleep \n");
    sleep_ESP32(TIME_TO_SLEEP);
}


