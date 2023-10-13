#ifndef __SD_ESP32_
#define __SD_ESP32_
// ----------------------------------------------------------------- //
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include <sys/unistd.h>
#include <sys/stat.h>

#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Define variables for SD CARD functions
#define PIN_SD_MISO         19
#define PIN_SD_MOSI         23
#define PIN_SD_CLK          18
#define PIN_SD_CS           5

#define MOUNT_POINT         "/sdcard"

#define file_salud_size     "/salud"
#define file_salud_data     "/sa_"
#define file_err_salud_size "/err_salud"
#define file_err_salud_dat  "/err_sa_"
#define file_pesaje_size    "/pesaje"
#define file_pesaje_data    "/pe_"

static const char* TAG_SD = "SD_CST";

/*
   Description:
   This function make available the read and write operations with the SD Card

   Returns:
   esp_err_t response = ESP_OK or ESP_FAIL
*/
esp_err_t init_SD(sdmmc_card_t **out_card, sdmmc_host_t *out_host);


/*
   Description:
   This function close communication with the SD Card

   Returns:
   Nothing
*/
void eject_SD(sdmmc_card_t *card, sdmmc_host_t *host);


/*
   Description:
   This function close communication with the SD Card

   Parameters:
   char* buffer : The content to storage
   char* name_file: The name of the file for that content

   Returns:
   esp_err_t response = ESP_OK or ESP_FAIL
*/
esp_err_t guardar_file_sd(char* buffer, char* name_file);


/**
 * @brief This function read the file content and storage in a buffers
 * @param name_file: const char pointer of the name of the file to read its data
 * @param buffer_read: char pointer of the buffer where it will storage the data readed
 * @param size_buffer: size_t variable of the size of data readed exactly
 */
size_t leer_file_sd(const char *name_file, char* buffer_read, size_t size_buffer);


/**
 * @brief This function delete a file in a SD card
 * @param name_file: const char pointer of the file name desired to delete
 */
esp_err_t delete_file_sd(const char *name_file);

bool file_exists(const char* file_path);
esp_err_t update_battery_file(char* localtime, float battery_level);

esp_err_t append_file_sd(char* buffer, char* name_file);

esp_err_t create_file(const char *name_file);

// ----------------------------------------------------------------- //
#endif /* __SD_ESP32_ */