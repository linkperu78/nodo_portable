#include "esp32_sd.h"

esp_err_t init_SD(sdmmc_card_t **out_card, sdmmc_host_t *out_host) {
    esp_err_t ret_sd;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 10,
        .allocation_unit_size = 8 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG_SD, "Initializing SD card");
    ESP_LOGI(TAG_SD, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    *out_host = host;
    host.max_freq_khz = 5000;   // Default freqz = 20 MHz (this value causes an issues with SD detector)
    //host.max_freq_khz = 10000;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };
    ret_sd = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret_sd != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to initialize bus.");
        return ret_sd;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG_SD, "Mounting filesystem");
    ret_sd = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret_sd != ESP_OK) {
        if (ret_sd == ESP_FAIL) {
            ESP_LOGE(TAG_SD, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG_SD, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret_sd));
        }
        return ret_sd;
    }
    ESP_LOGI(TAG_SD, "Tarjeta SD conectada exitosamente\n");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Set the card pointer in the calling function
    *out_card = card;

    return ESP_OK;
}


void eject_SD(sdmmc_card_t *card, sdmmc_host_t *host) {
    const char mount_point[] = MOUNT_POINT;

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG_SD, "Card unmounted");

    // Get the host from the card and deinitialize the bus after all devices are removed
    spi_bus_free(host->slot);
}

bool file_exists(const char* file_path){
    return (access(file_path, F_OK) == 0);
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


esp_err_t guardar_file_sd(char* buffer, char* name_file){
    char new_file_name[50];
    sprintf(new_file_name, "%s%s",MOUNT_POINT, name_file);
    FILE* f = fopen(new_file_name, "w");
    if (f == NULL) {
        ESP_LOGE(TAG_SD, "No se pudo escribir el archivo: %s\n", name_file);
        return ESP_FAIL;
    }
    fprintf(f, buffer);
    fclose(f);
    ESP_LOGI(TAG_SD, "Se escribio en el archivo: %s\n", name_file);
    return ESP_OK;
}


esp_err_t append_file_sd(char* buffer, char* name_file){
    char new_file_name[50];
    sprintf(new_file_name, "%s%s", MOUNT_POINT, name_file);
    FILE* f = fopen(new_file_name, "a"); // Open in append mode ("a")
    if (f == NULL) {
        ESP_LOGE(TAG_SD, "No se pudo escribir el archivo: %s\n", name_file);
        return ESP_FAIL;
    }
    fprintf(f, "%s", buffer); // Write the buffer contents
    fclose(f);
    ESP_LOGI(TAG_SD, "Se agrego la data %s en el archivo: %s\n",buffer, name_file);
    return ESP_OK;
}


size_t leer_file_sd(const char *name_file, char* buffer_read, size_t size_buffer)
{
    char new_file_name[50];
    sprintf(new_file_name, "%s%s",MOUNT_POINT, name_file);

    ESP_LOGI(TAG_SD, "Reading file %s", name_file);
    FILE* f = fopen(new_file_name, "r");
    if (f == NULL) {
        ESP_LOGE(TAG_SD, "No se pudo abrir el archivo para lectura: %s", name_file);
        sprintf(buffer_read, "%d",0);
        return 0;
    }

    fseek(f, 0, SEEK_END); // Move the file pointer to the end of the file
    size_t file_size = ftell(f);
    if( file_size >= size_buffer ){
        ESP_LOGE(TAG_SD, "Size of data readed [%d] >= size buffer [%d]\n", file_size, size_buffer);
        return 0;
    }
    if (file_size == -1) {
        ESP_LOGE(TAG_SD, "Failed to get file size: %s", name_file);
        return 0;
    }
    fclose(f);

    f = fopen(new_file_name, "r");
    size_t bytes_read;
    bytes_read = fread(buffer_read, 1, size_buffer, f);
    fclose(f);

    if (bytes_read == 0) {
        ESP_LOGE(TAG_SD, "No data read from file: %s", new_file_name);
        return 0;
    }

    return bytes_read;
}


esp_err_t delete_file_sd(const char *name_file) {
    char file_path[50];
    sprintf(file_path, "%s%s", MOUNT_POINT, name_file);

    // Use the remove() function to delete the file
    int result = remove(file_path);
    if (result != 0) {
        ESP_LOGE(TAG_SD, "Failed to delete the file: %s\n", name_file);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_SD, "File deleted successfully: %s\n", name_file);
    return ESP_OK;
}


esp_err_t create_file(const char *name_file){
    char file_path[50];
    sprintf(file_path, "%s%s", MOUNT_POINT, name_file);

    ESP_LOGE("SD_CARD", "Creating the folder: %s\n", name_file);
    FILE* f = fopen(name_file, "w");
    if (f == NULL) {
        ESP_LOGE(TAG_SD, "No se pudo crear \n");
        return ESP_FAIL;
    }
    fprintf(f, "%s", "0");
    fclose(f);
    return ESP_OK;
}