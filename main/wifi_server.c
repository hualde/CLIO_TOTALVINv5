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
#include "freertos/timers.h"

static const char *TAG = "wifi_server";

static char vin_vehiculo_global[18] = "No disponible";
static char vin_columna_global[18] = "No disponible";
static bool dtc_cleared = false;
static bool status_has_been_checked = false;
static bool real_status_loaded = false;
static bool calibracion_executed = false;
static int countdown_value = 5;

extern void clear_dtc_task(void *pvParameters);
extern void check_status_task(void *pvParameters);

// New function declarations
static void calibracion_angulo(void);
static void countdown_timer_callback(TimerHandle_t xTimer);
static void send_calibration_frames(void);

static esp_err_t http_server_handler(httpd_req_t *req)
{
    char resp_str[1000];
    const char* dtc_message = dtc_cleared ? "<p>DTC borrado exitosamente</p>" : "";
    const char* calibracion_message = calibracion_executed ? 
        "<p>Calibración de ángulo en progreso. Tiempo restante: <span id='countdown'></span></p>" : "";
    
    char status_str[100] = "";
    if (status_has_been_checked && real_status_loaded) {
        int current_status = get_global_status();
        snprintf(status_str, sizeof(status_str), "<p>Estado actual: %d</p>", current_status);
    } else if (status_has_been_checked && !real_status_loaded) {
        snprintf(status_str, sizeof(status_str), "<p>Cargando estado...</p>");
    }

    char combined_status[200];
    strncpy(combined_status, status_str, sizeof(combined_status));
    combined_status[sizeof(combined_status) - 1] = '\0';

    // Generate the HTML response with auto-refresh and countdown script
    snprintf(resp_str, sizeof(resp_str),
             "<html><head>"
             "<meta http-equiv='refresh' content='5'>"
             "<script>"
             "var countdownValue = %d;"
             "function updateCountdown() {"
             "  if (countdownValue > 0) {"
             "    document.getElementById('countdown').innerText = countdownValue;"
             "    countdownValue--;"
             "    setTimeout(updateCountdown, 1000);"
             "  } else {"
             "    document.getElementById('countdown').innerText = 'Completado';"
             "  }"
             "}"
             "updateCountdown();"
             "</script>"
             "</head><body>"
             "<h1>Informacion de VIN</h1>"
             "<p>VIN del vehiculo: %s</p>"
             "<p>VIN de la columna: %s</p>"
             "<form action='/clear_dtc' method='get'>"
             "<input type='submit' value='Borrar DTC'>"
             "</form>"
             "<form action='/check_status' method='get'>"
             "<input type='submit' value='Check Status'>"
             "</form>"
             "<form action='/calibracion_angulo' method='get'>"
             "<input type='submit' value='Calibración de Ángulo'>"
             "</form>"
             "%s"
             "%s"
             "%s"
             "</body></html>",
             countdown_value, vin_vehiculo_global, vin_columna_global, dtc_message, calibracion_message, combined_status);

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

static esp_err_t check_status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de verificación de estado desde el servidor web");
    xTaskCreate(check_status_task, "check_status_task", 2048, NULL, 5, NULL);
    
    status_has_been_checked = true;
    real_status_loaded = false;  // Reset this flag when starting a new check

    // Redirect to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t calibracion_angulo_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando calibración de ángulo");
    calibracion_angulo();
    
    calibracion_executed = true;
    countdown_value = 5;  // Reset countdown

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
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void calibracion_angulo(void)
{
    ESP_LOGI(TAG, "Iniciando calibración de ángulo");
    
    // Create and start a timer for the countdown
    TimerHandle_t countdown_timer = xTimerCreate("CountdownTimer", pdMS_TO_TICKS(1000), pdTRUE, NULL, countdown_timer_callback);
    if (countdown_timer != NULL) {
        xTimerStart(countdown_timer, 0);
    } else {
        ESP_LOGE(TAG, "Failed to create countdown timer");
    }
}

static void countdown_timer_callback(TimerHandle_t xTimer)
{
    if (countdown_value > 0) {
        ESP_LOGI(TAG, "Countdown: %d", countdown_value);
        countdown_value--;
    } else {
        ESP_LOGI(TAG, "Countdown finished. Sending CAN frames.");
        xTimerStop(xTimer, 0);
        xTimerDelete(xTimer, 0);
        send_calibration_frames();
        calibracion_executed = false;  // Reset the flag after sending frames
    }
}

static void send_calibration_frames(void)
{
    uint8_t frame1[] = {0x03, 0x14, 0xFF, 0x00, 0xB6, 0x01, 0xFF, 0xFF};
    uint8_t frame2[] = {0x03, 0x31, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame3[] = {0x03, 0x31, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};

    send_can_frame(0x742, frame1, 8);
    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay between frames
    send_can_frame(0x742, frame2, 8);
    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay between frames
    send_can_frame(0x742, frame3, 8);

    ESP_LOGI(TAG, "Calibration frames sent");
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
}