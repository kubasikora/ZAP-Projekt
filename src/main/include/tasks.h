#ifndef __TASKS_H_
#define __TASKS_H_

// uart buffer size
#define ECHO_BUFFER_SIZE (1024)

// server info
#define WEB_SERVER "dell"
#define WEB_PORT "8081"

extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;

void http_get_task(void *pvParameters);
void http_post_task(void *pvParameters);
void echo_task();

#endif //__TASKS_H_