#include "esp32_general.h"

void delay_ms(int time_in_ms){
  vTaskDelay( time_in_ms / portTICK_PERIOD_MS);
}


void activate_pin(int pin_number){
	gpio_set_level(pin_number, 1);	
	vTaskDelay(5 / portTICK_PERIOD_MS);	
}


void deactivate_pin(int pin_number){
	gpio_set_level(pin_number, 0);	
	vTaskDelay(5 / portTICK_PERIOD_MS);	
}


void power_off_leds(){
  led_set(BAT, OFF);
  led_set(WIFI, OFF);
  led_set(CHECK, OFF);
}


void power_on_leds(){
  led_set(BAT,WHITE);
  led_set(WIFI,WHITE);
  led_set(CHECK,WHITE);
}


void config_pin(){
  gpio_reset_pin(ESP_LED_WIFI_R);
  gpio_reset_pin(ESP_LED_WIFI_G);
  gpio_reset_pin(ESP_LED_WIFI_B);
  
  gpio_reset_pin(ESP_LED_CHECK_R);
  gpio_reset_pin(ESP_LED_CHECK_G);
  gpio_reset_pin(ESP_LED_CHECK_B);

  gpio_reset_pin(ESP_LED_BAT_R);
  gpio_reset_pin(ESP_LED_BAT_G);
  gpio_reset_pin(ESP_LED_BAT_B);

  gpio_reset_pin(PinSD);

  gpio_set_direction(ESP_LED_WIFI_R, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_WIFI_G, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_WIFI_B, GPIO_MODE_OUTPUT);

  gpio_set_direction(ESP_LED_CHECK_R, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_CHECK_G, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_CHECK_B, GPIO_MODE_OUTPUT);

  gpio_set_direction(ESP_LED_BAT_R, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_BAT_G, GPIO_MODE_OUTPUT);
  gpio_set_direction(ESP_LED_BAT_B, GPIO_MODE_OUTPUT);

  gpio_set_direction(PinSD, GPIO_MODE_OUTPUT);

  gpio_pulldown_en(ESP_LED_CHECK_R);
  gpio_pulldown_en(ESP_LED_CHECK_B);
  gpio_pulldown_en(ESP_LED_CHECK_G);
  gpio_pulldown_en(ESP_LED_BAT_R);
  gpio_pulldown_en(ESP_LED_BAT_B);
  gpio_pulldown_en(ESP_LED_BAT_G);
  gpio_pulldown_en(ESP_LED_WIFI_R);
  gpio_pulldown_en(ESP_LED_WIFI_B);
  gpio_pulldown_en(ESP_LED_WIFI_G);

  gpio_pullup_dis(ESP_LED_CHECK_R);
  gpio_pullup_dis(ESP_LED_CHECK_B);
  gpio_pullup_dis(ESP_LED_CHECK_G);
  gpio_pullup_dis(ESP_LED_BAT_R);
  gpio_pullup_dis(ESP_LED_BAT_B);
  gpio_pullup_dis(ESP_LED_BAT_G);
  gpio_pullup_dis(ESP_LED_WIFI_R);
  gpio_pullup_dis(ESP_LED_WIFI_B);
  gpio_pullup_dis(ESP_LED_WIFI_G);


  power_off_leds();
}


void sleep_ESP32(int _time_to_sleep){
  gpio_deep_sleep_hold_en();
  esp_sleep_pd_config( ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON );

  ESP_LOGI(TAG_ESP32, "El sistema entrara a deep sleep por %d minutos\n", _time_to_sleep);
  uint64_t time_to_sleep = _time_to_sleep * MIN_TO_S * S_TO_US;
    
  esp_sleep_enable_timer_wakeup(time_to_sleep);
  esp_deep_sleep_start();
	delay_ms(100);

}


void led_set(enum _led pin_led, enum _color color){
  int pin_R = 4;
  int pin_G = 4;
  int pin_B = 4;

  // Seleccionamos el led a cambiar de estado
  switch(pin_led){
    case WIFI:
      pin_R = ESP_LED_WIFI_R;
      pin_G = ESP_LED_WIFI_G;
      pin_B = ESP_LED_WIFI_B;
      break;
    case BAT:
      pin_R = ESP_LED_BAT_R;
      pin_G = ESP_LED_BAT_G;
      pin_B = ESP_LED_BAT_B;
      break;
    case CHECK:
      pin_R = ESP_LED_CHECK_R;
      pin_G = ESP_LED_CHECK_G;
      pin_B = ESP_LED_CHECK_B;
      break;
  }

  switch (color)
  {
    case OFF:
      deactivate_pin(pin_R);
      deactivate_pin(pin_G);
      deactivate_pin(pin_B);
      break;
    case RED:
      activate_pin(pin_R);
      deactivate_pin(pin_G);
      deactivate_pin(pin_B);
      break;
    case BLUE:
      deactivate_pin(pin_R);
      deactivate_pin(pin_G);
      activate_pin(pin_B);
      break;
    case GREEN:
      deactivate_pin(pin_R);
      activate_pin(pin_G);
      deactivate_pin(pin_B);
      break;
    case PURPLE:
      activate_pin(pin_R);
      deactivate_pin(pin_G);
      activate_pin(pin_B);
      break;
    case CIAN:
      deactivate_pin(pin_R);
      activate_pin(pin_G);
      activate_pin(pin_B);
      break;
    case YELLOW:
      activate_pin(pin_R);
      activate_pin(pin_G);
      deactivate_pin(pin_B);
      break;
    case WHITE:
      activate_pin(pin_R);
      activate_pin(pin_G);
      activate_pin(pin_B);
      break;
    default:
      deactivate_pin(pin_R);
      deactivate_pin(pin_G);
      deactivate_pin(pin_B);
      break;
  }
  delay_ms(200);
}


void log_free_space_esp32(){

  uint32_t free_heap_size = esp_get_free_heap_size();
  uint32_t total_heap_size = esp_get_minimum_free_heap_size() + free_heap_size;
  float percent_free = ((float)free_heap_size / total_heap_size) * 100.0;
  ESP_LOGI(TAG_ESP32, "[APP] Free memory: %lu bytes (%.2f%% free)", free_heap_size, percent_free);

}