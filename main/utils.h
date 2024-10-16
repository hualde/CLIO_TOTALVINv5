#ifndef UTILS_H
#define UTILS_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

void empty_task_199(void *pvParameters);
void restart_timer_callback(TimerHandle_t xTimer);

#endif // UTILS_H