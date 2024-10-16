#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

extern uint8_t stored_bytes_vehicle[VIN_LENGTH];
extern uint8_t stored_bytes_column[VIN_LENGTH];
extern int stored_bytes_count_vehicle;
extern int stored_bytes_count_column;
extern char vin_vehiculo[VIN_LENGTH + 1];
extern char vin_columna[VIN_LENGTH + 1];

extern EventGroupHandle_t vin_event_group;
extern TimerHandle_t restart_timer;

#endif // TYPES_H