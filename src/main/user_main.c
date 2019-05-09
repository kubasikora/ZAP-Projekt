#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include "driver/uart.h"

#include <netdb.h>
#include <sys/socket.h>
#include <string.h>

// echo task
#define ECHO_BUFFER_SIZE (1024)

// wifi credentials
#define WIFI_SSID "esp-test"
#define WIFI_PASS "esp-test"

// server info
#define WEB_SERVER "example.com"
#define WEB_PORT (80)
#define WEB_URL "http://example.com/"

static const char* REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

// FreeRTOS event group to signal when we are connected 
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static const char* TAG = "ZAP";

static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch(event->event_id){
      case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

      case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip: %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;

      case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station: "MACSTR" join, AID=%d", MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
        break;

      case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station: "MACSTR"leave, AID=%d", MAC2STR(event->event_info.sta_disconnected.mac), event->event_info.sta_disconnected.aid);
        break;

      case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;

      default:
        break;
    }
    return ESP_OK;
}


static void http_get_task(void *pvParameters){
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1){
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");
        
        // find IP of service
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);
        if (err != 0 || res == NULL){
            ESP_LOGE(TAG, "DNS lookup failed error = %d, res = %p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeded. IP=%s", inet_ntoa(*addr));
    }
}



static void echo_task(){
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(ECHO_BUFFER_SIZE);
    while (1){
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, ECHO_BUFFER_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
        uart_write_bytes(UART_NUM_0, (const char *) data, len);
    }
}


static void hello_task(){
    char buffer[30];
    while(1){
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        uint8_t length = sprintf(buffer, "Hello! Time: %d\n", esp_log_timestamp());
        uart_write_bytes(UART_NUM_0, buffer, length+1);
    }
}

void wifi_init_sta(){
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}


void uart_init(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, ECHO_BUFFER_SIZE * 2, 0, 0, NULL);
}


void app_main(){
    uart_init();
    wifi_init_sta();

    xTaskCreate(echo_task, "uart_echo_task", 1024, NULL, 10, NULL);
    xTaskCreate(hello_task, "hello_task", 1024, NULL, 10, NULL);
}