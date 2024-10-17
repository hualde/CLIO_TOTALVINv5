#include "can_communication.h"
#include "config.h"
#include "types.h"
#include "vin_handling.h"
#include "esp_log.h"
#include "driver/twai.h"
#include <string.h>
#include <ctype.h>
#include "wifi_server.h"  // Asegúrate de que este archivo exista y contenga la declaración de update_real_status

static const char *CAN_TAG = "CAN_COMMUNICATION";

static bool check_status_mode = false;
static int received_762_frames = 0;
#define MAX_762_FRAMES 10

// Global variable to store the status
static int global_status = 3;

static bool status_updated = false;

extern void update_real_status(int status);

void send_can_frame(uint32_t id, uint8_t *data, uint8_t dlc) {
    if (xEventGroupGetBits(vin_event_group) & STOP_TASKS_BIT) {
        return;  // No enviar si se ha señalizado la detención
    }

    twai_message_t message;
    message.identifier = id;
    message.extd = 0;  // Frame estándar (11 bits)
    message.rtr = 0;   // No es una solicitud de transmisión remota
    message.data_length_code = dlc;
    memcpy(message.data, data, dlc);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(1000));
    if (result != ESP_OK) {
        ESP_LOGE(CAN_TAG, "Error al enviar trama CAN: ID=0x%03" PRIx32 " Error: %s", id, esp_err_to_name(result));
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
}

void communication_task(void *pvParameters) {
    // Tramas para el vehículo
    uint8_t frame1[] = {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame2[] = {0x02, 0x21, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame3[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame4[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame5[] = {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame6[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame7[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame8[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame9[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame10[] = {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame11[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame12[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame13[] = {0x02, 0x21, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame14[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame15[] = {0x02, 0x21, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame16[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Tramas para la columna
    uint8_t frame17[] = {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame18[] = {0x02, 0x21, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame19[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame20[] = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame21[] = {0x02, 0x21, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame22[] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    while (1) {
        EventBits_t bits = xEventGroupGetBits(vin_event_group);
        if (bits & STOP_TASKS_BIT) {
            ESP_LOGI(CAN_TAG, "Deteniendo la tarea de comunicacion.");
            vTaskDelete(NULL);
        }

        if (!(bits & VIN_VEHICLE_BIT)) {
            // Enviar tramas para el vehículo
            send_can_frame(0x745, frame1, 8);
            send_can_frame(0x745, frame2, 8);
            send_can_frame(0x745, frame3, 8);
            send_can_frame(0x745, frame4, 8);
            send_can_frame(0x745, frame5, 8);
            send_can_frame(0x745, frame6, 8);
            send_can_frame(0x745, frame7, 8);
            send_can_frame(0x745, frame8, 8);
            send_can_frame(0x745, frame9, 8);
            send_can_frame(0x745, frame10, 8);
            send_can_frame(0x745, frame11, 8);
            send_can_frame(0x745, frame12, 8);
            send_can_frame(0x745, frame13, 8);
            send_can_frame(0x745, frame14, 8);
            send_can_frame(0x745, frame15, 8);
            send_can_frame(0x745, frame16, 8);
        } else if (!(bits & VIN_COLUMN_BIT)) {
            // Enviar tramas para la columna
            send_can_frame(0x742, frame17, 8);
            send_can_frame(0x742, frame18, 8);
            send_can_frame(0x742, frame19, 8);
            send_can_frame(0x742, frame20, 8);
            send_can_frame(0x742, frame21, 8);
            send_can_frame(0x742, frame22, 8);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void check_status_task(void *pvParameters) {
    uint8_t frames[][8] = {
        {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x02, 0x21, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x02, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x02, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    };

    status_updated = false;
    check_status_mode = true;
    received_762_frames = 0;
    global_status = 3;  // Reset status to 3 at the start of check

    for (int i = 0; i < 7; i++) {
        send_can_frame(0x742, frames[i], 8);
        vTaskDelay(pdMS_TO_TICKS(100));  // Esperar 100ms entre tramas
    }

    ESP_LOGI(CAN_TAG, "Tramas de check status enviadas");

    // Esperar un tiempo para recibir las tramas 762 o hasta que se reciban MAX_762_FRAMES
    int timeout = 0;
    while (received_762_frames < MAX_762_FRAMES && timeout < 100) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout++;
    }

    check_status_mode = false;
    ESP_LOGI(CAN_TAG, "Fin de la verificación de estado. Estado final: %d", global_status);
    vTaskDelete(NULL);
}

void receive_task(void *pvParameters) {
    while (1) {
        EventBits_t bits = xEventGroupGetBits(vin_event_group);
        if (bits & STOP_TASKS_BIT) {
            ESP_LOGI(CAN_TAG, "Deteniendo la tarea de recepcion.");
            vTaskDelete(NULL);
        }

        twai_message_t message;
        esp_err_t result = twai_receive(&message, pdMS_TO_TICKS(100));
        if (result == ESP_OK) {
            if (message.identifier == 0x762 && check_status_mode && !status_updated) {
                if (message.data[0] == 0x23 && message.data[1] == 0x00) {
                    ESP_LOGI(CAN_TAG, "Trama 762 filtrada recibida: [%02X %02X %02X %02X %02X %02X %02X %02X]",
                             message.data[0], message.data[1], message.data[2], message.data[3],
                             message.data[4], message.data[5], message.data[6], message.data[7]);
                    received_762_frames++;

                    // Check if byte 4 ends with 'C' (case insensitive)
                    char byte4_char = message.data[3] & 0x0F;  // Get the least significant nibble
                    if (byte4_char == 0x0C) {  // 0x0C is hexadecimal for 12, which represents 'C'
                        global_status = 4;
                    } else {
                        global_status = 3;
                    }
                    ESP_LOGI(CAN_TAG, "Estado actualizado: %d", global_status);
                    
                    // Call update_real_status with the new status
                    update_real_status(global_status);
                    status_updated = true;
                }
            }
            else if (message.identifier == TARGET_ID_1 || message.identifier == TARGET_ID_2) {
                uint8_t *target_stored_bytes;
                int *target_stored_bytes_count;
                char *target_VIN;
                EventBits_t vin_bit;
                bool is_vehicle;

                if (message.identifier == TARGET_ID_1 && !(bits & VIN_VEHICLE_BIT)) {
                    target_stored_bytes = stored_bytes_vehicle;
                    target_stored_bytes_count = &stored_bytes_count_vehicle;
                    target_VIN = vin_vehiculo;
                    vin_bit = VIN_VEHICLE_BIT;
                    is_vehicle = true;
                } else if (message.identifier == TARGET_ID_2 && !(bits & VIN_COLUMN_BIT)) {
                    target_stored_bytes = stored_bytes_column;
                    target_stored_bytes_count = &stored_bytes_count_column;
                    target_VIN = vin_columna;
                    vin_bit = VIN_COLUMN_BIT;
                    is_vehicle = false;
                } else {
                    continue;
                }

                if (message.data[0] == 0x10 && message.data_length_code >= 8) {
                    store_bytes(target_stored_bytes, target_stored_bytes_count, message.data, 4, 4);
                } 
                else if (message.data[0] == 0x21 && message.data_length_code == 8) {
                    store_bytes(target_stored_bytes, target_stored_bytes_count, message.data, 1, 7);
                } 
                else if (message.data[0] == 0x22 && message.data_length_code == 8) {
                    store_bytes(target_stored_bytes, target_stored_bytes_count, message.data, 1, 6);
                }

                // Cuando se han recibido los 17 bytes del VIN
                if (*target_stored_bytes_count == VIN_LENGTH) {
                    // Convertir los bytes almacenados a caracteres ASCII
                    for (int i = 0; i < VIN_LENGTH; i++) {
                        target_VIN[i] = (char)target_stored_bytes[i];
                    }
                    target_VIN[VIN_LENGTH] = '\0'; // Añadir el carácter nulo al final

                    if (validate_vin(target_VIN, is_vehicle)) {
                        if  (is_vehicle) {
                            ESP_LOGI(CAN_TAG, "VIN del vehiculo valido: %s", target_VIN);
                        } else {
                            ESP_LOGI(CAN_TAG, "VIN de la columna valido: %s", target_VIN);
                        }
                        xEventGroupSetBits(vin_event_group, vin_bit);
                    } else {
                        if (is_vehicle) {
                            ESP_LOGW(CAN_TAG, "VIN del vehiculo no valido: %s", target_VIN);
                        } else {
                            ESP_LOGW(CAN_TAG, "VIN de la columna no valido: %s", target_VIN);
                        }
                        *target_stored_bytes_count = 0; // Reiniciar para intentar de nuevo
                        continue;
                    }
                    
                    // Reiniciar el contador
                    *target_stored_bytes_count = 0;
                }
            }
        } else if (result != ESP_ERR_TIMEOUT) {
            ESP_LOGE(CAN_TAG, "Error al recibir trama CAN: %s", esp_err_to_name(result));
        }
    }
}

int get_global_status() {
    return global_status;
}