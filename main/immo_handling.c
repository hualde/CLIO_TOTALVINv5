#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>
#include "immo_handling.h"
#include "config.h"
#include "can_communication.h"
#include "esp_log.h"

void set_immo_task(void *pvParameters) {
    uint8_t data1[] = {0x02, 0x10, 0xC0, 0x08, 0x00, 0x00, 0x00, 0x08};
    send_can_frame(0x742, data1, 8);

    uint8_t data2[] = {0x02, 0x10, 0xFB, 0x08, 0x00, 0x00, 0x00, 0x08};
    send_can_frame(0x742, data2, 8);

    uint8_t data3[] = {0x02, 0x3B, 0x05, 0x08, 0x00, 0x00, 0x00, 0x08};
    send_can_frame(0x742, data3, 8);

    uint8_t data4[] = {0x02, 0x10, 0xFA, 0x0D, 0xB6, 0x01, 0xFF, 0xFF};
    send_can_frame(0x742, data4, 8);

    ESP_LOGI(TAG, "IMMO programado");
    vTaskDelete(NULL);
}