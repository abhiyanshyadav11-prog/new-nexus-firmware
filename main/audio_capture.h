
#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdint.h>

void audio_capture_init(void);
void audio_task(void *pvParameters);

#endif // AUDIO_CAPTURE_H
