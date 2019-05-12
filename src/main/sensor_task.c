#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/uart.h"
#include "driver/adc.h"
#include "driver/pwm.h"

#include"include/tasks.h"

float sensorValue;

static const char* TAG = "ADC";

void adc_task(void *pvParameters){
    const float A = 0.3101;
    const float B = -250.243;
    uint16_t adc_data[1];
    const float THRESHOLD = 25.0;

    while (1) {
        if (ESP_OK == adc_read(&adc_data[0])) {
            ESP_LOGI(TAG, "adc read: %d", adc_data[0]);
            xSemaphoreTake(xMutex, portMAX_DELAY);
            sensorValue =  adc_data[0]*A + B;
            xSemaphoreGive(xMutex);
            uint32_t duty = 0;        
            
            if (sensorValue > THRESHOLD){
                duty = (sensorValue - THRESHOLD)*50;
                if (duty > 500) {
                    duty = 500;
                }
                ESP_LOGI(TAG, "DUTY = %d", duty);
            }
            pwm_set_duties(&duty);
            pwm_start();
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}