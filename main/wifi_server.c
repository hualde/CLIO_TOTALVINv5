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
#include "can_communication.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "wifi_server";

static char vin_vehiculo_global[18] = "No disponible";
static char vin_columna_global[18] = "No disponible";
static bool dtc_cleared = false;
static bool status_has_been_checked = false;
static bool real_status_loaded = false;
static bool calibracion_instructions_shown = false;
static char dtc_message[100] = "";
static char status_message[100] = "Estado no verificado";

extern void clear_dtc_task(void *pvParameters);
extern void check_status_task(void *pvParameters);
extern void send_calibration_frames_task(void *pvParameters);

static esp_err_t http_server_handler(httpd_req_t *req)
{
    char resp_str[2000];

    // Generate the HTML response with improved styling and auto-refresh
    snprintf(resp_str, sizeof(resp_str),
             "<html><head>"
             "<meta charset='UTF-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1'>"
             "<meta http-equiv='refresh' content='5'>"  // Auto-refresh every 5 seconds
             "<style>"
             "body { font-family: Arial, sans-serif; line-height: 1.6; padding: 20px; max-width: 800px; margin: 0 auto; background-color: #f4f4f4; }"
             "h1 { color: #333; text-align: center; }"
             ".vin-info { background-color: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }"
             ".button { background-color: #E4007B; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 5px; }"
             ".button:hover { background-color: #C4006B; }"
             ".success-message { color: green; font-weight: bold; }"
             ".status { font-weight: bold; }"
             ".instructions { background-color: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-top: 20px; }"
             ".instructions h2 { color: #E4007B; }"
             ".instructions ol { padding-left: 20px; }"
             ".action-container { display: flex; align-items: center; margin-bottom: 10px; }"
             ".action-message { margin-left: 10px; }"
             "</style>"
             "</head><body>"
             "<h1>Informaci&oacute;n de VIN</h1>"
             "<div class='vin-info'>"
             "<p><strong>VIN del veh&iacute;culo:</strong> %s</p>"
             "<p><strong>VIN de la columna:</strong> %s</p>"
             "</div>"
             "<div class='action-container'>"
             "<form action='/clear_dtc' method='get'>"
             "<input type='submit' value='Borrar DTC' class='button'>"
             "</form>"
             "<span class='action-message'>%s</span>"
             "</div>"
             "<div class='action-container'>"
             "<form action='/check_status' method='get'>"
             "<input type='submit' value='Check Status' class='button'>"
             "</form>"
             "<span class='action-message'>%s</span>"
             "</div>"
             "<form action='/calibracion_angulo' method='get'>"
             "<input type='submit' value='Calibraci&oacute;n de &Aacute;ngulo' class='button'>"
             "</form>"
             "</body></html>",
             vin_vehiculo_global, vin_columna_global, dtc_message, status_message);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t clear_dtc_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de borrado de DTC desde el servidor web");
    xTaskCreate(clear_dtc_task, "clear_dtc_task", 2048, NULL, 5, NULL);
    
    dtc_cleared = true;
    snprintf(dtc_message, sizeof(dtc_message), "DTC borrado exitosamente");

    // Redirect to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t check_status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de verificación de estado desde el servidor web");
    xTaskCreate(check_status_task, "check_status_task", 2048, NULL, 5, NULL);
    
    status_has_been_checked = true;
    real_status_loaded = false;  // Reset this flag when starting a new check
    snprintf(status_message, sizeof(status_message), "Verificando estado...");

    // Redirect to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t calibracion_angulo_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Mostrando instrucciones de calibración de ángulo");
    calibracion_instructions_shown = true;

    // Instead of redirecting, we'll render the page with instructions
    char resp_str[2000];
    snprintf(resp_str, sizeof(resp_str),
             "<html><head>"
             "<meta charset='UTF-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1'>"
             "<style>"
             "body { font-family: Arial, sans-serif; line-height: 1.6; padding: 20px; max-width: 800px; margin: 0 auto; background-color: #f4f4f4; }"
             "h1, h2 { color: #333; }"
             ".button { background-color: #E4007B; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 5px; }"
             ".button:hover { background-color: #C4006B; }"
             ".instructions { background-color: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-top: 20px; }"
             "</style>"
             "</head><body>"
             "<h1>Calibraci&oacute;n de &Aacute;ngulo</h1>"
             "<div class='instructions'>"
             "<h2>Instrucciones de calibraci&oacute;n de &aacute;ngulo:</h2>"
             "<ol>"
             "<li>Con el motor encendido, ponga el volante/ruedas en el centro</li>"
             "<li>Gire el volante a la izquierda hasta el tope</li>"
             "<li>Gire el volante a la derecha hasta el tope</li>"
             "<li>Vuelva a centrar el volante/ruedas</li>"
             "<li>Pulse el bot&oacute;n 'Enviar tramas de calibraci&oacute;n'</li>"
             "<li>Una vez finalizado este proceso, apague el coche y vuelva a encenderlo</li>"
             "</ol>"
             "<form action='/send_calibration_frames' method='get'>"
             "<input type='submit' value='Enviar tramas de calibraci&oacute;n' class='button'>"
             "</form>"
             "</div>"
             "<br><a href='/' class='button'>Volver</a>"
             "</body></html>");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t send_calibration_frames_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando envío de tramas de calibración");
    xTaskCreate(send_calibration_frames_task, "send_calibration_frames_task", 2048, NULL, 5, NULL);

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

static httpd_uri_t check_status = {
    .uri       = "/check_status",
    .method    = HTTP_GET,
    .handler   = check_status_handler,
    .user_ctx  = NULL
};

static httpd_uri_t calibracion_angulo_uri = {
    .uri       = "/calibracion_angulo",
    .method    = HTTP_GET,
    .handler   = calibracion_angulo_handler,
    .user_ctx  = NULL
};

static httpd_uri_t send_calibration_frames_uri = {
    .uri       = "/send_calibration_frames",
    .method    = HTTP_GET,
    .handler   = send_calibration_frames_handler,
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
        httpd_register_uri_handler(server, &check_status);
        httpd_register_uri_handler(server, &calibracion_angulo_uri);
        httpd_register_uri_handler(server, &send_calibration_frames_uri);
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

void update_real_status(int status)
{
    real_status_loaded = true;
    snprintf(status_message, sizeof(status_message), "Estado actual: %d", status);
    ESP_LOGI(TAG, "Status updated: %s", status_message);
}