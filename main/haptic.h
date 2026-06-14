
#ifndef HAPTIC_H
#define HAPTIC_H

#include <stdint.h>

typedef enum {
    HAPTIC_PATTERN_NONE,
    HAPTIC_PATTERN_SHORT_BUZZ,
    HAPTIC_PATTERN_DOUBLE_BUZZ,
    HAPTIC_PATTERN_LONG_PULSE
} haptic_pattern_t;

void haptic_init(void);
void haptic_task(void *pvParameters);
void haptic_play_pattern(haptic_pattern_t pattern);

#endif // HAPTIC_H
