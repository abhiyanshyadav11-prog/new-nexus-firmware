
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "esp_pm.h"

void power_manager_init(void);
void power_manager_go_to_light_sleep_after_delay(uint32_t delay_ms);
void power_manager_wake_from_light_sleep(void);
void power_manager_go_to_deep_sleep(void);

#endif // POWER_MANAGER_H
