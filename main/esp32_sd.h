#ifndef __SD_ESP32_
#define __SD_ESP32_
// ----------------------------------------------------------------- //
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Librerias generales
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"

// Librerias para lectura SD
#include <sys/unistd.h>
#include <sys/stat.h> 
#include "esp_vfs_fat.h"   // provides support for the FAT filesystem, often used for reading and writing files on SD cards and other storage media.
#include "sdmmc_cmd.h"     // It provides command definitions for interacting with SD cards, issuing commands, and reading and writing data to and from SD cards.

#include <errno.h>


// Define variables for SD CARD functions
#define PIN_SD_MISO         19
#define PIN_SD_MOSI         23
#define PIN_SD_CLK          18
#define PIN_SD_CS           5

#define MOUNT_POINT           "/sdcard"

#define file_salud_size       "salud"
#define file_salud_data       "sa_"

//#define file_err_salud_size   "err_salud"
#define file_err_salud_size   "e_salud"

#define file_err_salud_dat    "e_sa_"
#define file_pesaje_size      "pesaje"
#define file_pesaje_data      "pe_"
#define file_bateria_data     "bateria"

#define TAG_SD                "SD_API"

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
esp_err_t eject_SD(sdmmc_card_t *card, sdmmc_host_t *host);


/*
   Description:
   This function save data in a file into a SD Card

   Parameters:
   char* buffer : The content to storage
   char* name_file: The name of the file for that content

   Returns:
   esp_err_t response = ESP_OK or ESP_FAIL
*/
esp_err_t guardar_file_sd(char* buffer, char* name_file);


/*
   Description:
   This function read data from the sd card and storage it into a buffer

   Parameters:
   char*    name_file : The file name that we'll read
   char*    buffer_read: The buffer for storage the data in the file
   size_t   size_buffer: The size of the buffer (for avoid overflow in the buffer)

   Returns:
   esp_err_t response = ESP_OK or ESP_FAIL
*/
size_t leer_file_sd(const char *name_file, char* buffer_read, size_t size_buffer);


/**
 * @brief This function delete a file in a SD card
 * @param name_file: const char pointer of the file name desired to delete
 */
esp_err_t delete_file_sd(const char *name_file);


int file_exists(const char* file_path);


esp_err_t append_file_sd(char* buffer, char* name_file);


esp_err_t create_file(const char *name_file, const char* initial_content);


// ----------------------------------------------------------------- //
#endif /* __SD_ESP32_ */