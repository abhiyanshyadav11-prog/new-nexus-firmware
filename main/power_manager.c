
#include "power_manager.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "POWER_MANAGER";

void power_manager_init(void)
{
    // Configure wake up sources
    esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, 0); // Wake up on button press (low level)
    esp_sleep_enable_timer_wakeup(30 * 1000000); // 30 seconds timer wake for BLE keepalive

    ESP_LOGI(TAG, "Power manager initialized");
}

void power_manager_go_to_light_sleep_after_delay(uint32_t delay_ms)
{
    ESP_LOGI(TAG, "Entering light sleep in %lu ms", delay_ms);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    esp_light_sleep_start();
    ESP_LOGI(TAG, "Woke up from light sleep");
}

void power_manager_wake_from_light_sleep(void)
{
    // This function is called when an interrupt wakes the ESP32 from light sleep
    // No explicit action needed here, as the system resumes execution
    ESP_LOGI(TAG, "Explicit wake from light sleep (e.g., button press)");
}

void power_manager_go_to_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}
