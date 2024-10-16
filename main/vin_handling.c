#include "vin_handling.h"
#include "config.h"
#include "types.h"
#include "can_communication.h"
#include "esp_log.h"
#include <string.h>

void store_bytes(uint8_t *stored_bytes, int *stored_bytes_count, uint8_t *data, int start, int length) {
    for (int i = 0; i < length && *stored_bytes_count < VIN_LENGTH; i++) {
        stored_bytes[(*stored_bytes_count)++] = data[start + i];
    }
}

bool validate_vin(const char *vin, bool is_vehicle) {
    if (strlen(vin) != VIN_LENGTH) {
        return false;
    }
    
    for (int i = 0; i < VIN_LENGTH; i++) {
        if (vin[i] == '\0' || vin[i] == ' ') {
            return false;
        }
    }
    
    if (is_vehicle && strncmp(vin, "VF1", 3) != 0) {
        return false;
    }
    
    return true;
}

void set_vin_task(void *pvParameters) {
    uint8_t data1[] = {0x02, 0x10, 0xC0, 0x30, 0x30, 0x30, 0x30, 0x30};
    send_can_frame(0x742, data1, 8);

    uint8_t data2[8] = {0x10, 0x15, 0x3B, 0x81, 0, 0, 0, 0};
    memcpy(data2 + 4, vin_vehiculo, 4);
    send_can_frame(0x742, data2, 8);

    uint8_t data3[8] = {0x21, 0, 0, 0, 0, 0, 0, 0};
    memcpy(data3 + 1, vin_vehiculo + 4, 7);
    send_can_frame(0x742, data3, 8);

    uint8_t data4[8] = {0x22, 0, 0, 0, 0, 0, 0, 0};
    memcpy(data4 + 1, vin_vehiculo + 11, 6);
    data4[7] = 0x49;
    send_can_frame(0x742, data4, 8);

    uint8_t data5[] = {0x23, 0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_can_frame(0x742, data5, 8);

    uint8_t data6[] = {0x02, 0x10, 0xFA, 0x30, 0x30, 0x30, 0x30, 0x30};
    send_can_frame(0x742, data6, 8);

    ESP_LOGI(TAG, "VIN establecido");
    vTaskDelete(NULL);
}