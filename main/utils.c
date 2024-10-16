#include "utils.h"
#include "config.h"
#include "can_communication.h"
#include "esp_log.h"
#include "esp_system.h"

void empty_task_199(void *pvParameters) {
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_can_frame(0x199, data, 8);
    ESP_LOGI(TAG, "Tarea vacía con ID 199 ejecutada");
    vTaskDelete(NULL);
}

void restart_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Reiniciando el microcontrolador...");
    esp_restart();
}