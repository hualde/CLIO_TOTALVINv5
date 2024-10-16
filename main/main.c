#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "types.h"
#include "can_communication.h"
#include "vin_handling.h"
#include "immo_handling.h"
#include "dtc_handling.h"
#include "utils.h"
#include "wifi_server.h"

// Global variables
uint8_t stored_bytes_vehicle[VIN_LENGTH];
uint8_t stored_bytes_column[VIN_LENGTH];
int stored_bytes_count_vehicle = 0;
int stored_bytes_count_column = 0;
char vin_vehiculo[VIN_LENGTH + 1];
char vin_columna[VIN_LENGTH + 1];

EventGroupHandle_t vin_event_group;
TimerHandle_t restart_timer;

void app_main(void) {
    vin_event_group = xEventGroupCreate();

    // Initialize Wi-Fi and web server
    init_wifi_server();

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = RX_QUEUE_SIZE;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI instalado");
    } else {
        ESP_LOGE(TAG, "Error al instalar el driver TWAI");
        return;
    }

    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI iniciado");
    } else {
        ESP_LOGE(TAG, "Error al iniciar el driver TWAI");
        return;
    }

    // Ejecutar la tarea vacía con ID 199 antes de comprobar VINs
    xTaskCreate(empty_task_199, "empty_task_199", 2048, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo para asegurar que la tarea se ejecute

    xTaskCreate(communication_task, "communication_task", 2048, NULL, 5, NULL);
    xTaskCreate(receive_task, "receive_task", 2048, NULL, 6, NULL);

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(vin_event_group, VIN_VEHICLE_BIT | VIN_COLUMN_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        if ((bits & (VIN_VEHICLE_BIT | VIN_COLUMN_BIT)) == (VIN_VEHICLE_BIT | VIN_COLUMN_BIT)) {
            ESP_LOGI(TAG, "VIN del vehiculo: %s", vin_vehiculo);
            ESP_LOGI(TAG, "VIN de la columna: %s", vin_columna);
            ESP_LOGI(TAG, "Ambos VINs obtenidos.");
            
            // Comparar VINs
            if (strcmp(vin_vehiculo, vin_columna) == 0) {
                ESP_LOGI(TAG, "Los VINs son iguales");
                
                // Crear el temporizador para reiniciar cada hora
                restart_timer = xTimerCreate("RestartTimer", pdMS_TO_TICKS(3600000), pdTRUE, 0, restart_timer_callback);
                
                if (restart_timer != NULL) {
                    // Iniciar el temporizador
                    xTimerStart(restart_timer, 0);
                    
                    // Iniciar la tarea de borrado de fallos cada 30 segundos
                    while (1) {
                        xTaskCreate(clear_dtc_task, "clear_dtc_task", 2048, NULL, 5, NULL);
                        vTaskDelay(pdMS_TO_TICKS(30000));  // Esperar 30 segundos
                    }
                } else {
                    ESP_LOGE(TAG, "No se pudo crear el temporizador de reinicio");
                }
            } else {
                ESP_LOGI(TAG, "Los VINs son diferentes");
                
                // Crear y ejecutar la tarea para establecer el VIN
                xTaskCreate(set_vin_task, "set_vin_task", 2048, NULL, 5, NULL);
                
                // Esperar a que la tarea termine
                vTaskDelay(pdMS_TO_TICKS(5000));  // Esperar 5 segundos
                
                // Crear y ejecutar la tarea para establecer el código IMMO
                xTaskCreate(set_immo_task, "set_immo_task", 2048, NULL, 5, NULL);
                
                // Esperar a que la tarea termine
                vTaskDelay(pdMS_TO_TICKS(5000));  // Esperar 5 segundos
                
                // Señalizar a las tareas que deben detenerse
                xEventGroupSetBits(vin_event_group, STOP_TASKS_BIT);
                
                // Esperar a que las tareas se detengan
                vTaskDelay(pdMS_TO_TICKS(2000));  // Aumentado a 2 segundos
                
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Detener y desinstalar el driver TWAI
    ESP_LOGI(TAG, "Deteniendo el driver TWAI...");
    if (twai_stop() == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI detenido");
    } else {
        ESP_LOGE(TAG, "Error al detener el driver TWAI");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar un segundo más

    if (twai_driver_uninstall() == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI desinstalado");
    } else {
        ESP_LOGE(TAG, "Error al desinstalar el driver TWAI");
    }

    vEventGroupDelete(vin_event_group);
    ESP_LOGI(TAG, "Programa finalizado");
}