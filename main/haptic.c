
#include "haptic.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "config.h"

static const char *TAG = "HAPTIC";

static QueueHandle_t haptic_queue;

void haptic_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 5000,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = VIBRATION_MOTOR_GPIO,
        .duty           = 0, // Set duty to 0% initially
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    haptic_queue = xQueueCreate(5, sizeof(haptic_pattern_t));
    if (haptic_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create haptic queue");
    }

    ESP_LOGI(TAG, "Haptic feedback initialized");
}

void haptic_task(void *pvParameters)
{
    haptic_pattern_t pattern;

    while (1) {
        if (xQueueReceive(haptic_queue, &pattern, portMAX_DELAY) == pdPASS) {
            switch (pattern) {
                case HAPTIC_PATTERN_NONE:
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    break;
                case HAPTIC_PATTERN_SHORT_BUZZ:
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512); // 50% duty cycle
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    break;
                case HAPTIC_PATTERN_DOUBLE_BUZZ:
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    break;
                case HAPTIC_PATTERN_LONG_PULSE:
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}

void haptic_play_pattern(haptic_pattern_t pattern)
{
    if (xQueueSend(haptic_queue, &pattern, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to send haptic pattern to queue");
    }
}
