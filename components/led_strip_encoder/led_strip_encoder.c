#include "esp_check.h"
#include "led_strip_encoder.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"

static const char *TAG = "led_strip_encoder";

#define WS2812_T0H_NS  350
#define WS2812_T0L_NS  800
#define WS2812_T1H_NS  700
#define WS2812_T1L_NS  600
#define WS2812_TRS_NS  50000 /*!< Reset code period */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    size_t encoded_bytes = 0;
    // Encode all the RGB data
    encoded_bytes += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
    // Add a reset code at the end
    encoded_bytes += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code, sizeof(rmt_symbol_word_t), &session_state);
    *ret_state = session_state;
    return encoded_bytes;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;
    ESP_GOTO_ON_FALSE(config, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = config->resolution * WS2812_T0H_NS / 1000000000,
            .level1 = 0,
            .duration1 = config->resolution * WS2812_T0L_NS / 1000000000,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = config->resolution * WS2812_T1H_NS / 1000000000,
            .level1 = 0,
            .duration1 = config->resolution * WS2812_T1L_NS / 1000000000,
        },
        .flags.msb_first = 1 // WS2812 expect MSB first
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = config->resolution * WS2812_TRS_NS / 1000000000,
        .level1 = 0,
        .duration1 = config->resolution * WS2812_TRS_NS / 1000000000,
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;
err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}
