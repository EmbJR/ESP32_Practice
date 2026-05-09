#include <stdio.h>
#include "esp_wifi.h"
#include "string.h"
#include "esp_log.h"
#include "esp_http_server.h"

static const char* TAG = "main";

#define ESP_WIFI_SSID "JBR_ESP"
#define ESP_WIFI_PASS "test_1234"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 2

const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Device Control</title>
    <style>
        body { font-family: sans-serif; display: flex; justify-content: center; padding-top: 50px; background-color: #f0f0f0; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.2); width: 250px; }
        label { font-weight: bold; }
        input { width: 100%; padding: 8px; margin: 10px 0; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        button { width: 100%; padding: 10px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #0056b3; }
    </style>
</head>
<body>
    <div class="card">
        <label for="duty">Duty:</label>
        <input type="number" id="duty" placeholder="Integer value" step="1">
        
        <label for="period">Period:</label>
        <input type="number" id="period" placeholder="Integer value" step="1">
        
        <button onclick="sendData()">Submit</button>
    </div>

    <script>
        function sendData() {
            const dutyVal = document.getElementById('duty').value;
            const periodVal = document.getElementById('period').value;

            if (dutyVal === "" || periodVal === "") {
                alert("Please fill in both fields.");
                return;
            }

            // Construct the JSON payload with integer values 
            const data = {
                "Duty": parseInt(dutyVal, 10),
                "Period": parseInt(periodVal, 10)
            };

            // POST request sent to the specific URI: /led/period [cite: 3, 5]
            fetch('/led/period', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' }, // [cite: 9, 16]
                body: JSON.stringify(data)
            })
            .then(response => {
                if (response.ok) alert("Settings Updated!");
                else alert("Server Error: " + response.status);
            })
            .catch(err => alert("Request Failed: " + err));
        }
    </script>
</body>
</html>
)rawliteral";
// ...

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data){
    printf("Event nr: %ld!\n", event_id);
}

void wifi_init_softap()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // always start with this

    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };


    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}

httpd_handle_t start_webserver() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Server started successfully, registering URI handlers...");
        return server;
    }

    ESP_LOGE(TAG, "Failed to start server");
    return NULL;
}

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t led_get_on_handler(httpd_req_t *req)
{
    const char* resp_str = "<h1>LED ON</h1>";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t led_get_off_handler(httpd_req_t *req)
{
    const char* resp_str = "<h1>LED OFF</h1>";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t led_period_set(httpd_req_t *req)
{
    char *token;
    int period = 0;
    int duty = 0;
    char buf[100];

    int ret = httpd_req_recv(req, buf, sizeof(buf));
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Socket timeout");
        }
        return ESP_FAIL;
    }

    buf[ret] = '\0'; // Null-terminate the received data
    ESP_LOGI(TAG, "Received LED period: %s", buf);
    
    // Get the first token
    token = strtok(buf, ":");
    if (token != NULL)
    {
        token = strtok(NULL, ",");
        period = atoi(token);
        token = strtok(NULL, ":");
        token = strtok(NULL, "}");
        duty = atoi(token);
    }
    
    ESP_LOGI(TAG, "------Received LED period: %d, duty: %d\n", period, duty);
    const char* resp_str = "<h1>LED Period Set</h1>";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static const httpd_uri_t hello_world_uri = {
    .uri     = "/",
    .method   = HTTP_GET,
    .handler  = hello_get_handler,
    .user_ctx = NULL
};

static const httpd_uri_t led_on_uri = {
    .uri     = "/led/on",
    .method   = HTTP_GET,
    .handler  = led_get_on_handler,
    .user_ctx = NULL
};

static const httpd_uri_t led_off_uri = {
    .uri     = "/led/off",
    .method   = HTTP_GET,
    .handler  = led_get_off_handler,
    .user_ctx = NULL
};

static const httpd_uri_t led_period_uri = {
    .uri     = "/led/period",
    .method   = HTTP_POST,
    .handler  = led_period_set,
    .user_ctx = NULL
};

 void app_main(void)
{
    wifi_init_softap();

    httpd_handle_t server = start_webserver();
    if (server) {
        ESP_LOGI(TAG, "Web server started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }

    httpd_register_uri_handler(server, &hello_world_uri);
    httpd_register_uri_handler(server, &led_on_uri);
    httpd_register_uri_handler(server, &led_off_uri);
    httpd_register_uri_handler(server, &led_period_uri);
}