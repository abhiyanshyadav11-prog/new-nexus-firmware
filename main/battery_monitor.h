
#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

void battery_monitor_init(void);
void battery_task(void *pvParameters);
uint8_t battery_get_level(void);

#endif // BATTERY_MONITOR_H
