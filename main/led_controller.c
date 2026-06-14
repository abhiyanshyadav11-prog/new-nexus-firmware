
#include "led_controller.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static led_pattern_t current_pattern = LED_PATTERN_OFF;

static const char *TAG = "LED_CONTROLLER";

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static QueueHandle_t led_pattern_queue;

static uint8_t led_strip_pixels[3]; // R, G, B for one LED

static void set_pixel(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_pixels[0] = g;
    led_strip_pixels[1] = r;
    led_strip_pixels[2] = b;
}

static void update_led_strip(void)
{
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no loop
    };
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
}

void led_controller_init(void)
{
    ESP_LOGI(TAG, "Initializing LED controller");

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_APB,
        .gpio_num = LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = 10000000, // 10MHz
        .trans_queue_depth = 4,
        
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    led_strip_encoder_config_t encoder_config = {
        .resolution = tx_chan_config.resolution_hz,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    led_pattern_queue = xQueueCreate(1, sizeof(led_pattern_t));
    if (led_pattern_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create LED pattern queue");
    }

    set_pixel(0, 0, 0); // Start with LED off
    update_led_strip();

    ESP_LOGI(TAG, "LED controller initialized");
}

void led_task(void *pvParameters)
{
    led_pattern_t current_pattern = LED_PATTERN_OFF;
    led_pattern_t new_pattern;

    while (1) {
        if (xQueueReceive(led_pattern_queue, &new_pattern, 0) == pdPASS) {
            current_pattern = new_pattern;
            ESP_LOGI(TAG, "LED pattern changed to %d", current_pattern);
        }

        switch (current_pattern) {
            case LED_PATTERN_OFF:
                set_pixel(0, 0, 0);
                break;
            case LED_PATTERN_CONNECTED_IDLE: // Solid Cyan #00D4FF
                set_pixel(0x00, 0xD4, 0xFF);
                break;
            case LED_PATTERN_LISTENING: { // Pulse Blue #0077FF
                for (int i = 0; i < 256; i += 5) {
                    set_pixel(0, 0, i);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                for (int i = 255; i >= 0; i -= 5) {
                    set_pixel(0, 0, i);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;
            }
            case LED_PATTERN_COMMAND_ACCEPTED: { // Flash Green x2 #00C896
                for (int i = 0; i < 2; i++) {
                    set_pixel(0x00, 0xC8, 0x96);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    set_pixel(0, 0, 0);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                current_pattern = LED_PATTERN_CONNECTED_IDLE; // Return to idle after flash
                break;
            }
            case LED_PATTERN_COMMAND_FAILED: { // Flash Red x3 #FF4444
                for (int i = 0; i < 3; i++) {
                    set_pixel(0xFF, 0x44, 0x44);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    set_pixel(0, 0, 0);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                current_pattern = LED_PATTERN_CONNECTED_IDLE; // Return to idle after flash
                break;
            }
            case LED_PATTERN_PROCESSING: { // Slow pulse Purple #7B2FBE
                for (int i = 0; i < 256; i += 2) {
                    set_pixel(i, 0, i);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                for (int i = 255; i >= 0; i -= 2) {
                    set_pixel(i, 0, i);
                    update_led_strip();
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                break;
            }
            case LED_PATTERN_BATTERY_CRITICAL: // Solid Red #FF0000
                set_pixel(0xFF, 0x00, 0x00);
                break;
            default:
                set_pixel(0, 0, 0);
                break;
        }
        update_led_strip();
        vTaskDelay(pdMS_TO_TICKS(50)); // Small delay to prevent busy-waiting
    }
    vTaskDelete(NULL);
}

void led_set_pattern(led_pattern_t pattern)
{
    current_pattern = pattern;

    if (xQueueSend(led_pattern_queue, &pattern, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to send LED pattern to queue");
    }
}

led_pattern_t led_get_current_pattern(void)
{
    return current_pattern;
}