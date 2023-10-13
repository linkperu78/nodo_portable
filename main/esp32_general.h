#ifndef __GENERAL_ESP32_
//--------------------------------------------------

#define __GENERAL_ESP32_
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "nvs_flash.h"          // For NVS (Non-volatile storage) - keep your data in shutdown
#include "esp_log.h"            // 

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_sleep.h"

#include "driver/adc.h"
//#include "esp_adc_cal.h"



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
#define PinSD               33

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
 */
/**
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
 * @brief This functions show the free space in bytes size and percentage
 */
void log_free_space_esp32();


void print_bytes(const char* string_to_display, size_t size_string);

//--------------------------------------------------
#endif /* __GENERAL_ESP32_ */