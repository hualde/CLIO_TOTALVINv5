#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#define EXAMPLE_ESP_WIFI_SSID      "Lizarte Clio"
#define EXAMPLE_ESP_WIFI_PASS      "RLizarteClio"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

void init_wifi_server(void);
void update_vin_data(const char* vin_vehiculo, const char* vin_columna);
void update_real_status(int status);

#endif // WIFI_SERVER_H