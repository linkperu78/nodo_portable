#ifndef __GENERAL_ESP32_
//--------------------------------------------------

#define __GENERAL_ESP32_
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#define TIME_TO_SLEEP   10
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
#define BatPin              32
#define PinSD               33

static const char* TAG_ESP32 = "General_CST";

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


//--------------------------------------------------
#endif /* __GENERAL_ESP32_ */