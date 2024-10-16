#ifndef CONFIG_H
#define CONFIG_H

#define TAG "TWAI_EXAMPLE"
#define TARGET_ID_1 0x765  // ID objetivo para filtrar las tramas CAN del vehículo
#define TARGET_ID_2 0x762  // ID objetivo para filtrar las tramas CAN de la columna
#define TX_GPIO_NUM 18   // GPIO para transmisión CAN
#define RX_GPIO_NUM 19   // GPIO para recepción CAN
#define RX_QUEUE_SIZE 64 // Tamaño aumentado del buffer de recepción
#define VIN_LENGTH 17    // Longitud del VIN

#define VIN_VEHICLE_BIT BIT0
#define VIN_COLUMN_BIT BIT1
#define STOP_TASKS_BIT BIT2

#endif // CONFIG_H