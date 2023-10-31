#ifndef __GENERAL_ESP32_
//--------------------------------------------------
#define __GENERAL_ESP32_

#include <string.h>             // Functions like strcpy, strlen, strcmp, and strcat  
#include <stdint.h>             // Defines various integer types with specific widths, such as int8_t, int16_t, int32_t, and so on. 
#include <stdio.h>              // It includes functions for reading and writing files, console input and output, and formatting data for output. 
#include <time.h>               // It allows you to get the current time, format time strings, calculate time differences, and perform various time-related operations.
#include <ctype.h>              // It includes functions like isalpha, isdigit, islower, toupper, and tolower for character classification and manipulation.
#include <stdlib.h>             // Includes functions for memory allocation and manipulation. Functions like malloc, free, atoi, and rand 
#include <sys/time.h>           // It includes functions for measuring time intervals and system time.

#include "freertos/FreeRTOS.h"  // It provides a framework for multitasking, task scheduling, and synchronization in embedded applications.
#include "freertos/task.h"      // Header provides functions and macros for creating, starting, and managing tasks
#include "freertos/event_groups.h" // This library is used for creating and managing event groups.
#include "soc/soc_caps.h"       // Allowing you to access and configure specific hardware features and functionality of the ESP32.
#include "nvs_flash.h"          // For NVS (Non-volatile storage) - keep your data in shutdown
#include "esp_log.h"            // For logs functions like ESP_LOGE, LOGI, etc
#include "esp_system.h"         // Includes functions for system initialization, rebooting, and retrieving system information

#include "driver/gpio.h"        // Can set up and control GPIO pins, configure input or output modes, set pin levels, and more.
#include "driver/uart.h"        // Allows you to configure and use UART peripherals on the ESP32, send and receive data over UART
#include "esp_sleep.h"          // It allows you to put the ESP32 into deep sleep or other low-power modes to save power when the device is idle.

#include "driver/adc.h"         // U can access the functions and features provided by this library to work with the ADC of the ESP32 microcontroller.


// For Deep Sleep Mode
#define TIME_TO_SLEEP   3
#define S_TO_US         1000000
#define MIN_TO_S        60

// Define LEDs pinout
#define ESP_LED_BAT_R       13
#define ESP_LED_BAT_G       12 
#define ESP_LED_BAT_B       14
#define ESP_LED_WIFI_R      21
#define ESP_LED_WIFI_G      17  
#define ESP_LED_WIFI_B      16
#define ESP_LED_CHECK_R     4
#define ESP_LED_CHECK_G     2
#define ESP_LED_CHECK_B     15

// SD Card Slot
#define PinSD               33

// ADC for battery
#define BAT_ADC_CHANNEL     ADC1_CHANNEL_4

// SALUD_MODE = 0, PESAJE_MODE = 1, 
enum _mode{
  SALUD_MODE  = 0,
  PESAJE_MODE = 1
};

// OFF=0, RED=1, GREEN=2, BLUE=3, PURPLE=4, CIAN=5, YELLOW=6, WHITE=7
enum _color{
  OFF = 0, RED = 1, GREEN = 2, BLUE = 3, PURPLE = 4,
  CIAN = 5, YELLOW = 6, WHITE = 7
};

// BAT = 1, WIFIF = 2, CHECK = 3
enum _led{
  BAT = 1,
  WIFI = 2,
  CHECK = 3
};


/**
 * @brief This function introduces a delay of the desired duration in milliseconds.
 * @param time_in_ms The time to wait in milliseconds.
 * @note Example Usage:
 * @code
 * delay_ms(2570); // Delays execution for 2.57 seconds.
 * @endcode
 */
void delay_ms(int time_in_ms);


/**
 * @brief This functions turn on GPIO pin of ESP32
 * @param pin_number: GPIO pin number
 */
void activate_pin(int pin_number);


/**
 * @brief This functions turn off GPIO pin of ESP32
 * @param pin_number: GPIO pin number
 */
void deactivate_pin(int pin_number);


/**
 * @brief This function turn off all leds: BAT, WIFI, CHECK
 */
void power_off_leds();


/**
 * @brief This function turn on all leds: BAT, WIFI, CHECK
 */
void power_on_leds();


/**
 * @brief This function configure the pins for GPIO Functions
 */
void config_pin();


/**
 * @brief This function turn the led in the color desired
 * @param _led: WIFI, BAT, CHECK
 * @param _color: OFF, RED, GREEN, BLUE, PURPLE, CIAN, YELLOW, WHITE
 */
void led_set(enum _led pin_led, enum _color color);


/**
 * @brief This functions turn on the deep sleep mode of the ESP32
 * @param _time_to_sleep: Duration of deep sleep for ESP32 in minutes
 */
void sleep_ESP32(int _time_to_sleep);


/**
 * @brief This function configures the ADC Channel of ESP32
 * @param adc_channel : number of channel
 */
void ADC_Channel_configure(int adc_channel);


/**
 * @brief This function return the value of voltage of a ADC channel
 * @param adc_channel : number of channel
 */
float adc_get_value(int adc_channel);


/**
 * @brief This function print a string in its bytes format
 */
void print_bytes(const char* string_to_display, size_t size_string);

//--------------------------------------------------
#endif /* __GENERAL_ESP32_ */