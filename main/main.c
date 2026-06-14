
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ble_manager.h"
#include "audio_capture.h"
#include "led_controller.h"
#include "button_handler.h"
#include "haptic.h"
#include "battery_monitor.h"
#include "power_manager.h"
#include "config.h"
#include "nvs_flash.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Nexus Pendant Firmware Starting...");

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize peripherals and services
    power_manager_init();
    ble_manager_init();
    audio_capture_init();
    led_controller_init();
    button_handler_init();
    haptic_init();
    battery_monitor_init();

    // Create FreeRTOS tasks
    xTaskCreate(ble_task, "ble_task", BLE_TASK_STACK_SIZE, NULL, BLE_TASK_PRIORITY, NULL);
    xTaskCreate(audio_task, "audio_task", AUDIO_TASK_STACK_SIZE, NULL, AUDIO_TASK_PRIORITY, NULL);
    xTaskCreate(led_task, "led_task", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL);
    xTaskCreate(haptic_task, "haptic_task", HAPTIC_TASK_STACK_SIZE, NULL, HAPTIC_TASK_PRIORITY, NULL);
    xTaskCreate(battery_task, "battery_task", BATTERY_TASK_STACK_SIZE, NULL, BATTERY_TASK_PRIORITY, NULL);

    // Main loop (can be used for high-level state management or go to sleep)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Example: Check for power state changes or handle global events
    }
}
