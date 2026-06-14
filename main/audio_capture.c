
#include "audio_capture.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "config.h"

static const char *TAG = "AUDIO_CAPTURE";

static i2s_chan_handle_t rx_chan;
static QueueHandle_t audio_queue;

void audio_capture_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_GPIO,
            .ws   = I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    audio_queue = xQueueCreate(AUDIO_QUEUE_SIZE, AUDIO_BUFFER_SIZE);
    if (audio_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create audio queue");
    }
    ESP_LOGI(TAG, "Audio capture initialized");
}

void audio_task(void *pvParameters)
{
    size_t bytes_read;
    int16_t *audio_buffer = (int16_t *)malloc(AUDIO_BUFFER_SIZE);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
    }

    while (1) {
        // Read data from I2S bus
        esp_err_t ret = i2s_channel_read(rx_chan, audio_buffer, AUDIO_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
        if (ret == ESP_OK && bytes_read > 0) {
            // TODO: Implement VAD (Voice Activity Detection) here
            // For now, just send all audio data to the queue
            if (xQueueSend(audio_queue, audio_buffer, 0) != pdPASS) {
                ESP_LOGW(TAG, "Audio queue full, dropping data");
            }
        } else if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error reading I2S: %s", esp_err_to_name(ret));
        }
    }
    free(audio_buffer);
    vTaskDelete(NULL);
}

// Function to get audio data from the queue (for BLE transmission)
QueueHandle_t get_audio_queue(void)
{
    return audio_queue;
}
