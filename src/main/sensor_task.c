#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/adc.h"
#include "driver/pwm.h"

#include"include/tasks.h"

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
            uint32_t duty = 0;        
            
            if (sensorValue > 20.0){
                duty = (sensorValue - 20.0)*50;
                if (duty > 500) {
                    duty = 500;
                }
                ESP_LOGI(TAG, "DUTY = %d", duty);
            }
            pwm_set_duties(&duty);
            pwm_start();
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}