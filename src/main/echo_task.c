#include "freertos/FreeRTOS.h"

#include "esp_system.h"
#include "driver/uart.h"

#include "include/tasks.h"

void echo_task(void *pvParameters){
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(ECHO_BUFFER_SIZE);
    while (1){
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, ECHO_BUFFER_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
        uart_write_bytes(UART_NUM_0, (const char *) data, len);
    }
}