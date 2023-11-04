#include "esp32_sd.h"
#include "credenciales.h"

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


esp_err_t eject_SD(sdmmc_card_t *card, sdmmc_host_t *host) {
    const char mount_point[] = MOUNT_POINT;

    esp_err_t unmount_result = esp_vfs_fat_sdcard_unmount(mount_point, card);
    if (unmount_result != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to unmount SD card");
        return unmount_result; // Return ESP_ERR with the specific error code
    }

    // Deinitialize the bus after all devices are removed
    esp_err_t bus_free_result = spi_bus_free(host->slot);
    if (bus_free_result != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to free SPI bus");
        return bus_free_result; // Return ESP_ERR with the specific error code
    }
    
    return ESP_OK; // Return ESP_OK when everything is successful
}


int file_exists(const char* file_name){
    char file_path[50];
    sprintf(file_path, "%s/%s", MOUNT_POINT, file_name);
    return (access(file_path, F_OK) == 0 ? 1 : 0);
}



esp_err_t guardar_file_sd(char* buffer, char* name_file){
    char new_file_name[50];
    sprintf(new_file_name, "%s/%s",MOUNT_POINT, name_file);
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
    memset(buffer_read, 0, size_buffer);
    sprintf(new_file_name, "%s/%s",MOUNT_POINT, name_file);

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
    sprintf(file_path, "%s/%s", MOUNT_POINT, name_file);

    // Use the remove() function to delete the file
    int result = remove(file_path);
    if (result != 0) {
        ESP_LOGE(TAG_SD, "Failed to delete the file: %s\n", name_file);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_SD, "File deleted successfully: %s\n", name_file);
    return ESP_OK;
}


// This function create a new file with initial content
// WARNING: name_file can NOT exceed 8 characters
esp_err_t create_file(const char *name_file, const char* initial_content){
    char file_path[50];
    sprintf(file_path, "%s/%s", MOUNT_POINT, name_file);

    ESP_LOGI(TAG_SD, "Creating the folder: %s", name_file);

    FILE* f = fopen(file_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG_SD, "\t\tNo se pudo crear el archivo : '%s' \n", file_path);
        return ESP_FAIL;
    }

    fprintf(f, "%s", initial_content);
    fclose(f);
    
    ESP_LOGI(TAG_SD, "\t\t File created: '%s' \n", name_file);
    return ESP_OK;
}


