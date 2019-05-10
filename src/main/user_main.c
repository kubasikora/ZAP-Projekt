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
#define WEB_SERVER "dell"
#define WEB_PORT "8081"
#define GET_WEB_URL "/"
#define POST_WEB_URL "/data"


static const char* GET_REQUEST = "GET " GET_WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static const char* POST_REQUEST = "POST " POST_WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

// FreeRTOS event group to signal when we are connected 
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static const char* TAG = "ZAP";

static esp_err_t event_handler(void *ctx, system_event_t *event){
    static const char* TAG = "EVENT HANDLER";
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
    static const char *TAG = "HTTP_GET_TASK";
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
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
        if (err != 0 || res == NULL){
            ESP_LOGE(TAG, "DNS lookup failed error = %d, res = %p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeded. IP=%s", inet_ntoa(*addr));

        // Create socket
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0){
            ESP_LOGE(TAG, "Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Allocated socket");

        // Connect to socket
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0){
            ESP_LOGE(TAG, "Failed to connect with socket, errno = %d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "connected to %s", GET_WEB_URL);
        freeaddrinfo(res);

        // Write request to socket
        if(write(s, GET_REQUEST, strlen(GET_REQUEST)) < 0){
            ESP_LOGE(TAG, "Failed to send to socket");
            close(s);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Socket send success");

        // Set socket receiving timeout
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0){
            ESP_LOGE(TAG, "Failed to set socket receiving output");
            close(s);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Set socket receiving timeout success");

        // Read HTTP response 
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++){
                putchar(recv_buf[i]); //print response
            }
        } while(r > 0);
        ESP_LOGI(TAG, "Done reading from socket");
        close(s);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Starting again!");
    }
}


static void http_post_task(void *pvParameters){
    static const char *TAG = "HTTP_POST_TASK";
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
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
        if (err != 0 || res == NULL){
            ESP_LOGE(TAG, "DNS lookup failed error = %d, res = %p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeded. IP=%s", inet_ntoa(*addr));

        // Create socket
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0){
            ESP_LOGE(TAG, "Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Allocated socket");

        // Connect to socket
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0){
            ESP_LOGE(TAG, "Failed to connect with socket, errno = %d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "connected to %s", POST_WEB_URL);
        freeaddrinfo(res);

        // Write request to socket
        if(write(s, POST_REQUEST, strlen(POST_REQUEST)) < 0){
            ESP_LOGE(TAG, "Failed to send to socket");
            close(s);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Socket send success");

        // Set socket receiving timeout
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0){
            ESP_LOGE(TAG, "Failed to set socket receiving output");
            close(s);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Set socket receiving timeout success");

        // Read HTTP response 
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++){
                putchar(recv_buf[i]); //print response
            }
        } while(r > 0);
        ESP_LOGI(TAG, "Done reading from socket");
        close(s);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Starting again!");
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID: %s password: %s", WIFI_SSID, WIFI_PASS);
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
    xTaskCreate(http_get_task, "http_get_task", 4096, NULL, 10, NULL);
    xTaskCreate(http_post_task, "http_post_task", 4096, NULL, 10, NULL);
}