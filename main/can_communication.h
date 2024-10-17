#ifndef CAN_COMMUNICATION_H
#define CAN_COMMUNICATION_H

#include <stdint.h>

void send_can_frame(uint32_t id, uint8_t *data, uint8_t dlc);
void communication_task(void *pvParameters);
void check_status_task(void *pvParameters);
void receive_task(void *pvParameters);
int get_global_status(void);

#endif // CAN_COMMUNICATION_H