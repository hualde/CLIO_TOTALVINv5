#ifndef VIN_HANDLING_H
#define VIN_HANDLING_H

#include <stdbool.h>
#include <stdint.h>

void store_bytes(uint8_t *stored_bytes, int  *stored_bytes_count, uint8_t *data, int start, int length);
bool validate_vin(const char *vin, bool is_vehicle);
void set_vin_task(void *pvParameters);

#endif // VIN_HANDLING_H