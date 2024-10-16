#include "wifi_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "wifi_server";

static char vin_vehiculo_global[18] = "No disponible";
static char vin_columna_global[18] = "No disponible";
static bool dtc_cleared = false;

extern void clear_dtc_task(void *pvParameters);

static esp_err_t http_server_handler(httpd_req_t *req)
{
    char resp_str[500];
    const char* dtc_message = dtc_cleared ? "<p>DTC borrado exitosamente</p>" : "";
    dtc_cleared = false; // Reset the flag

    snprintf(resp_str, sizeof(resp_str),
             "<html><body>"
             "<h1>Informacion de VIN</h1>"
             "<p>VIN del vehiculo: %s</p>"
             "<p>VIN de la columna: %s</p>"
             "<form action='/clear_dtc' method='get'>"
             "<input type='submit' value='Borrar DTC'>"
             "</form>"
             "%s"
             "</body></html>",
             vin_vehiculo_global, vin_columna_global, dtc_message);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t clear_dtc_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de borrado de DTC desde el servidor web");
    xTaskCreate(clear_dtc_task, "clear_dtc_task", 2048, NULL, 5, NULL);
    
    dtc_cleared = true;

    // Redirect to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = http_server_handler,
    .user_ctx  = NULL
};

static httpd_uri_t clear_dtc = {
    .uri       = "/clear_dtc",
    .method    = HTTP_GET,
    .handler   = clear_dtc_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &clear_dtc);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void init_wifi_server(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    start_webserver();
}

void update_vin_data(const char* vin_vehiculo, const char* vin_columna)
{
    strncpy(vin_vehiculo_global, vin_vehiculo, sizeof(vin_vehiculo_global) - 1);
    vin_vehiculo_global[sizeof(vin_vehiculo_global) - 1] = '\0';

    strncpy(vin_columna_global, vin_columna, sizeof(vin_columna_global) - 1);
    vin_columna_global[sizeof(vin_columna_global) - 1] = '\0';

    ESP_LOGI(TAG, "VIN data updated. Vehiculo: %s, Columna: %s", vin_vehiculo_global, vin_columna_global);
}
