#include "led_controller.h"
#include "battery_monitor.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "ble_manager.h"
#include "led_controller.h"

static const char *TAG = "BATTERY_MONITOR";

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_calibration = false;

// Function to check if calibration is supported and perform it
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is AWP_ADC_CALI_SCHEME_CURVE_FITTING");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, out_handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is AWP_ADC_CALI_SCHEME_LINE_FITTING");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, out_handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void battery_monitor_init(void)
{
    //-------------ADC1 Init-----------------
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    //-------------ADC1 Config-----------------
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11, // Full range of 0-3.3V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATTERY_ADC_CHANNEL, &config));

    do_calibration = adc_calibration_init(ADC_UNIT_1, BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11, &adc_cali_handle);

    ESP_LOGI(TAG, "Battery monitor initialized");
}

uint8_t battery_get_level(void)
{
    int adc_raw;
    int voltage = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTERY_ADC_CHANNEL, &adc_raw));

    if (do_calibration) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage));
    } else {
        // If no calibration, use a rough estimation (e.g., based on 3.3V reference)
        // This needs to be adjusted based on actual hardware and voltage divider if any
        voltage = (adc_raw * 3300) / 4095; // Assuming 12-bit ADC and 3.3V reference
    }

    // Convert voltage to percentage (LiPo 3.7V nominal, range ~3.0V - 4.2V)
    // This is a simplified linear approximation, a more accurate lookup table or curve fitting can be used
    uint8_t percentage = 0;
    if (voltage >= 4200) percentage = 100;
    else if (voltage <= 3000) percentage = 0;
    else percentage = (uint8_t)(((float)(voltage - 3000) / (4200 - 3000)) * 100);

    ESP_LOGD(TAG, "Raw ADC: %d, Voltage: %d mV, Battery: %d%%", adc_raw, voltage, percentage);

    return percentage;
}

void battery_task(void *pvParameters)
{
    uint8_t battery_level;
    while (1) {
        battery_level = battery_get_level();
        ble_send_battery_level(battery_level);

        if (battery_level < 10) {
            led_set_pattern(LED_PATTERN_BATTERY_CRITICAL);
        } else {
            // Reset LED pattern if it was critical and now it's not (or if it was already idle)
            // This logic might need refinement based on overall state machine
            // For now, assume it goes back to connected idle if not critical
            led_set_pattern(LED_PATTERN_CONNECTED_IDLE);
        }

        vTaskDelay(pdMS_TO_TICKS(BATTERY_MONITOR_INTERVAL_MS));
    }
    vTaskDelete(NULL);
}
