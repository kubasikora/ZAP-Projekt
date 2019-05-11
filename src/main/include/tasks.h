#ifndef __TASKS_H_
#define __TASKS_H_

#include "freertos/event_groups.h"

// uart buffer size
#define ECHO_BUFFER_SIZE (1024)

// server info
#define WEB_SERVER "dell"
#define WEB_PORT "8081"

extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;
extern float sensorValue;

void http_get_task(void *pvParameters);
void http_post_task(void *pvParameters);
void echo_task(void *pvParameters);
void adc_task(void *pvParameters);

#endif //__TASKS_H_