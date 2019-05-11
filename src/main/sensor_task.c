#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/adc.h"

float sensorValue;

static const char* TAG = "ADC";

void adc_task(void *pvParameters){
    const float A = 0.135;
    const float B = -47.1;
    uint16_t adc_data[100];

    while (1) {
        if (ESP_OK == adc_read(&adc_data[0])) {
            ESP_LOGI(TAG, "adc read: %d", adc_data[0]);
            sensorValue =  adc_data[0]*A + B;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}