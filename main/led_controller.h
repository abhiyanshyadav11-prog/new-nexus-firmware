
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>

typedef enum {
    LED_PATTERN_OFF,
    LED_PATTERN_CONNECTED_IDLE, // Solid Cyan
    LED_PATTERN_LISTENING,      // Pulse Blue
    LED_PATTERN_COMMAND_ACCEPTED, // Flash Green x2
    LED_PATTERN_COMMAND_FAILED, // Flash Red x3
    LED_PATTERN_PROCESSING,     // Slow pulse Purple
    LED_PATTERN_BATTERY_CRITICAL // Solid Red
} led_pattern_t;

void led_controller_init(void);
void led_task(void *pvParameters);
void led_set_pattern(led_pattern_t pattern);
led_pattern_t led_get_current_pattern(void);

#endif // LED_CONTROLLER_H
