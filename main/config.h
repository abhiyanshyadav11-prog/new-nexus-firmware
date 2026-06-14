
#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
#include "driver/i2s_types.h"
#include "esp_adc/adc_oneshot.h"

// Pin Assignments
#define I2S_BCLK_GPIO           GPIO_NUM_7
#define I2S_WS_GPIO             GPIO_NUM_8
#define I2S_DIN_GPIO            GPIO_NUM_9
#define LED_GPIO                GPIO_NUM_10
#define BUTTON_GPIO             GPIO_NUM_0
#define VIBRATION_MOTOR_GPIO    GPIO_NUM_11
#define OLED_SDA_GPIO           GPIO_NUM_21
#define OLED_SCL_GPIO           GPIO_NUM_22
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_3   // GPIO4

// Audio Configuration
#define SAMPLE_RATE             16000
#define I2S_NUM_0               I2S_NUM_0
#define AUDIO_BUFFER_SIZE       (1024 * sizeof(int16_t)) // 1KB buffer for 16-bit samples
#define AUDIO_QUEUE_SIZE        5 // Number of audio buffers in queue

// Task Stack Sizes and Priorities
#define BLE_TASK_STACK_SIZE         6144
#define BLE_TASK_PRIORITY           4
#define AUDIO_TASK_STACK_SIZE       8192
#define AUDIO_TASK_PRIORITY         3
#define LED_TASK_STACK_SIZE         4096
#define LED_TASK_PRIORITY           2
#define HAPTIC_TASK_STACK_SIZE      2048
#define HAPTIC_TASK_PRIORITY        2
#define BATTERY_TASK_STACK_SIZE     2048
#define BATTERY_TASK_PRIORITY       1
#define BUTTON_TASK_STACK_SIZE      2048
#define BUTTON_TASK_PRIORITY        5

// Battery Monitoring
#define BATTERY_MONITOR_INTERVAL_MS 60000 // 60 seconds

#endif // CONFIG_H
