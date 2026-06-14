
#include "button_handler.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "config.h"
#include "power_manager.h"
#include "led_controller.h"

static const char *TAG = "BUTTON_HANDLER";

// Queue for button events
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void button_task(void* arg)
{
    uint32_t io_num;
    TickType_t press_start_time = 0;
    bool button_pressed = false;

    for(;;)
    {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (gpio_get_level(io_num) == 0) { // Button pressed (active low)
                if (!button_pressed) {
                    press_start_time = xTaskGetTickCount();
                    button_pressed = true;
                    ESP_LOGI(TAG, "Button pressed on GPIO %lu", (unsigned long)io_num);
                    // Wake up from sleep if in light sleep
                    power_manager_wake_from_light_sleep();
                    led_set_pattern(LED_PATTERN_LISTENING);
                }
            } else { // Button released
                if (button_pressed) {
                    TickType_t press_duration = xTaskGetTickCount() - press_start_time;
                    button_pressed = false;
                    ESP_LOGI(TAG,
                             "Button released on GPIO %lu, duration: %lu ms",
                             (unsigned long)io_num,
                             (unsigned long)(press_duration * portTICK_PERIOD_MS));
                    led_set_pattern(LED_PATTERN_CONNECTED_IDLE);
                    // TODO: Implement short/long press logic here
                    // For now, after release, if not deep sleep, go to light sleep after a delay
                    power_manager_go_to_light_sleep_after_delay(3000);
                }
            }
        }
    }
}

void button_handler_init(void)
{
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE; // Detect both press and release
    //bit mask of the pins, use BUTTON_GPIO_PIN
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(button_task, "button_task", BUTTON_TASK_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);

    ESP_LOGI(TAG, "Button handler initialized on GPIO %d", BUTTON_GPIO);
}
