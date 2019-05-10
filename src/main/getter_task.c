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

#include"include/tasks.h"

#define GET_WEB_URL "/"

static const char* GET_REQUEST = "GET " GET_WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

void http_get_task(void *pvParameters){
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

        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Starting again!");
    }
}