#include "dtc_handling.h"
#include "config.h"
#include "can_communication.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>

void clear_dtc_task(void *pvParameters) {
    uint8_t data[] = {0x03, 0x14, 0xFF, 0x00, 0xB6, 0x01, 0xFF, 0xFF};
    send_can_frame(0x742, data, 8);

    ESP_LOGI(TAG, "Senal de borrado de fallos enviada");
    vTaskDelete(NULL);
}