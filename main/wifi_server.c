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

static char current_language[3] = "es";

typedef struct {
    const char* title;
    const char* vehicle_vin;
    const char* column_vin;
    const char* angle_calibration;
    const char* clear_faults;
    const char* check_angle_calibration;
    // Añade aquí más campos según sea necesario
} language_strings_t;

static const language_strings_t languages[] = {
    {
        .title = "Lizarte Clio Configuración",
        .vehicle_vin = "VIN del vehículo:",
        .column_vin = "VIN de la columna:",
        .angle_calibration = "Calibración del ángulo",
        .clear_faults = "Borrar Fallos",
        .check_angle_calibration = "Comprobar calibración de ángulo"
    },
    {
        .title = "Lizarte Clio Configuration",
        .vehicle_vin = "Vehicle VIN:",
        .column_vin = "Column VIN:",
        .angle_calibration = "Angle Calibration",
        .clear_faults = "Clear Faults",
        .check_angle_calibration = "Check angle calibration"
    },
    {
        .title = "Configuration Lizarte Clio",
        .vehicle_vin = "VIN du véhicule:",
        .column_vin = "VIN de la colonne:",
        .angle_calibration = "Calibration de l'angle",
        .clear_faults = "Effacer les défauts",
        .check_angle_calibration = "Vérifier la calibration de l'angle"
    }
};

static const language_strings_t* get_language_strings() {
    if (strcmp(current_language, "en") == 0) {
        return &languages[1];
    } else if (strcmp(current_language, "fr") == 0) {
        return &languages[2];
    }
    return &languages[0]; // Default to Spanish
}

static esp_err_t http_server_handler(httpd_req_t *req)
{
    const language_strings_t* lang = get_language_strings();

    char resp_str[2500];
    snprintf(resp_str, sizeof(resp_str),
             "<html><head>"
             "<meta charset='UTF-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1'>"
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
             ".status-box { padding: 5px 10px; border-radius: 5px; font-weight: bold; }"
             ".status-4 { background-color: #4CAF50; color: white; }"
             ".status-3 { background-color: #FFEB3B; color: black; }"
             ".language-selector { margin-bottom: 20px; text-align: right; }"
             "select { padding: 5px; font-size: 16px; border-radius: 5px; }"
             "</style>"
             "</head><body>"
             "<div class='language-selector'>"
             "<select id='language' onchange='changeLanguage()'>"
             "<option value='es' %s>Español</option>"
             "<option value='en' %s>English</option>"
             "<option value='fr' %s>Français</option>"
             "</select>"
             "</div>"
             "<h1>%s</h1>"
             "<div class='vin-info'>"
             "<p><strong>%s</strong> %s</p>"
             "<p><strong>%s</strong> %s</p>"
             "</div>"
             "<div class='action-container'>"
             "<form action='/calibracion_angulo' method='get'>"
             "<input type='submit' value='%s' class='button'>"
             "</form>"
             "</div>"
             "<div class='action-container'>"
             "<form action='/clear_dtc' method='get'>"
             "<input type='submit' value='%s' class='button'>"
             "</form>"
             "<span class='action-message'>%s</span>"
             "</div>"
             "<div class='action-container'>"
             "<form action='/check_status' method='get'>"
             "<input type='submit' value='%s' class='button'>"
             "</form>"
             "</div>"
             "<script>"
             "function changeLanguage() {"
             "  var lang = document.getElementById('language').value;"
             "  window.location.href = '/change_language?lang=' + lang;"
             "}"
             "</script>"
             "</body></html>",
             strcmp(current_language, "es") == 0 ? "selected" : "",
             strcmp(current_language, "en") == 0 ? "selected" : "",
             strcmp(current_language, "fr") == 0 ? "selected" : "",
             lang->title, lang->vehicle_vin, vin_vehiculo_global, lang->column_vin, vin_columna_global,
             lang->angle_calibration, lang->clear_faults, dtc_message, lang->check_angle_calibration);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t clear_dtc_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de borrado de DTC desde el servidor web");
    xTaskCreate(clear_dtc_task, "clear_dtc_task", 2048, NULL, 5, NULL);
    
    dtc_cleared = true;
    snprintf(dtc_message, sizeof(dtc_message), "Los fallos se han borrado");

    // Redirect to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t check_status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Mostrando estado calibracion");
    status_has_been_checked = false;
    real_status_loaded = false;

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
             ".status-container { background-color: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-top: 20px; }"
             ".status-box { padding: 5px 10px; border-radius: 5px; font-weight: bold; display: inline-block; margin-top: 10px; }"
             ".status-4 { background-color: #4CAF50; color: white; }"
             ".status-3 { background-color: #FFEB3B; color: black; }"
             "</style>"
             "</head><body>"
             "<h1>Verificación de calibración de ángulo</h1>"
             "<div class='status-container'>"
             "<h2>Estado actual:</h2>"
             "<p id='status-message'>No se ha realizado ninguna verificación</p>"
             "<form action='/perform_status_check' method='get'>"
             "<input type='submit' value='Realizar verificacion' class='button'>"
             "</form>"
             "</div>"
             "<br><a href='/' class='button'>Volver</a>"
             "<script>"
             "function checkStatus() {"
             "  fetch('/get_status')"
             "    .then(response => response.text())"
             "    .then(data => {"
             "      if (data !== 'No se ha realizado ninguna verificación') {"
             "        document.getElementById('status-message').innerHTML = data;"
             "      }"
             "    });"
             "}"
             "setInterval(checkStatus, 5000);"
             "</script>"
             "</body></html>");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
   return ESP_OK;
}

static esp_err_t perform_status_check_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Iniciando tarea de verificación de estado");
    xTaskCreate(check_status_task, "check_status_task", 2048, NULL, 5, NULL);
    
    // Redirect back to the check_status page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/check_status");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t get_status_handler(httpd_req_t *req)
{
    const char* status_class = (get_global_status() == 4) ? "status-4" : (get_global_status() == 3) ? "status-3" : "";
    char status_str[100];
    snprintf(status_str, sizeof(status_str), 
             "<span class='status-box %s'>Status %d</span>", 
             status_class, get_global_status());

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, status_str, strlen(status_str));
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
             "<h1>Calibración de ángulo</h1>"
             "<div class='instructions'>"
             "<h2>Instrucciones de calibración de ángulo:</h2>"
             "<ol>"
             "<li>Con el motor encendido, ponga el volante/ruedas en el centro</li>"
             "<li>Gire el volante a la izquierda hasta el tope</li>"
             "<li>Gire el volante a la derecha hasta el tope</li>"
             "<li>Vuelva a centrar el volante/ruedas</li>"
             "<li>Pulse el botón 'Calibrar'</li>"
             "<li>Una vez finalizado este proceso, apague el coche y vuelva a encenderlo</li>"
             "</ol>"
             "<form action='/send_calibration_frames' method='get'>"
             "<input type='submit' value='Calibrar' class='button'>"
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

static esp_err_t change_language_handler(httpd_req_t *req)
{
    char lang[3];
    char buf[100];
    int ret, remaining = req->content_len;

    if ((ret = httpd_req_get_url_query_str(req, buf, sizeof(buf))) == ESP_OK) {
        if (httpd_query_key_value(buf, "lang", lang, sizeof(lang)) == ESP_OK) {
            ESP_LOGI(TAG, "Changing language to: %s", lang);
            strncpy(current_language, lang, sizeof(current_language) - 1);
            current_language[sizeof(current_language) - 1] = '\0';
        }
    }

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static httpd_uri_t change_language = {
    .uri       = "/change_language",
    .method    = HTTP_GET,
    .handler   = change_language_handler,
    .user_ctx  = NULL
};

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

static httpd_uri_t perform_status_check = {
    .uri       = "/perform_status_check",
    .method    = HTTP_GET,
    .handler   = perform_status_check_handler,
    .user_ctx  = NULL
};

static httpd_uri_t get_status = {
    .uri       = "/get_status",
    .method    = HTTP_GET,
    .handler   = get_status_handler,
    .user_ctx  = NULL
};

static httpd_uri_t calibracion_angulo_uri = {
    .uri       = "/calibracion_angulo",
    .method    = HTTP_GET,
    .handler   = calibracion_angulo_handler,
    .user_ctx  = NULL
};

static httpd_uri_t send_calibration_frames_uri = {
    .uri       = 

 "/send_calibration_frames",
    .method    = HTTP_GET,
    .handler   = send_calibration_frames_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &clear_dtc);
        httpd_register_uri_handler(server, &check_status);
        httpd_register_uri_handler(server, &perform_status_check);
        httpd_register_uri_handler(server, &get_status);
        httpd_register_uri_handler(server, &calibracion_angulo_uri);
        httpd_register_uri_handler(server, &send_calibration_frames_uri);
        httpd_register_uri_handler(server, &change_language);
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
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, 
             EXAMPLE_ESP_WIFI_CHANNEL);
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
    const char* status_class = (status == 4) ? "status-4" : (status == 3) ? "status-3" : "";
    snprintf(status_message, sizeof(status_message), 
             "<span class='status-box %s'>Status %d</span>", 
             status_class, status);
    ESP_LOGI(TAG, "Status updated: %d", status);
}