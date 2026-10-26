#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "task.h"
#include "serial.h"
#include "semphr.h"

#define BUFFER_SIZE 20

uint32_t buffer[BUFFER_SIZE];
uint32_t byteCount = 0;

SemaphoreHandle_t SemCountingFull;
SemaphoreHandle_t SemCountingEmpty;
SemaphoreHandle_t SemBinary;

//*****************************************************************************
//
// Task which adds items to the shared buffer.
// Each task has to take a counting semaphore for empty slots to be able to
// start their work. It then locks the shared resource and after it's work is 
// done it releases the shared resource and gives a "full" counting semaphore.
//
//*****************************************************************************
static void producer(void *pvParameters) {
        const TickType_t xDelay = pdMS_TO_TICKS(100); // 100 ms delay
        
        for (;;) {
                if (SemCountingEmpty != NULL && SemCountingFull != NULL) {
                        if (xSemaphoreTake(SemCountingEmpty, portMAX_DELAY) == pdTRUE){
                                if (xSemaphoreTake(SemBinary, portMAX_DELAY) == pdTRUE){
                                        buffer[byteCount++] = 1;
                                        UARTSend("Producer added item to buffer\n", strlen("Producer added item to buffer\n")); 
                                        char empty_buffer[30];
                                        int len = snprintf(empty_buffer, sizeof(empty_buffer), "Empty Semaphores: %d!\n", BUFFER_SIZE - byteCount);
                                        if (len > 0 && len < sizeof(empty_buffer)) {
                                                UARTSend((uint8_t *) empty_buffer, len);
                                        }
                                        xSemaphoreGive(SemBinary);
                                }
                                xSemaphoreGive(SemCountingFull);
                        }
                }
                vTaskDelay(xDelay);
        }
}

//*****************************************************************************
//
// Task which removes items from the shared buffer.
// Each task has to take a counting semaphore for full slots to be able to
// start their work. It then locks the shared resource and after it's work is 
// done it releases the shared resource and gives a "empty" counting semaphore.
//
//*****************************************************************************
static void consumer(void *pvParameters) {
        const TickType_t xDelay = pdMS_TO_TICKS(500); // 500ms delay

        for (;;) {
                if (SemCountingEmpty != NULL && SemCountingFull != NULL) {
                        if (xSemaphoreTake(SemCountingFull, portMAX_DELAY) == pdTRUE){
                                if (xSemaphoreTake(SemBinary, portMAX_DELAY) == pdTRUE){
                                        buffer[byteCount--] = 0; 
                                        UARTSend("Consumer took item from buffer\n", strlen("Consumer took item from buffer\n"));
                                        char full_buffer[30];
                                        int len = snprintf(full_buffer, sizeof(full_buffer), "Full Semaphores: %d!\n", byteCount);
                                        if (len > 0 && len < sizeof(full_buffer)) {
                                                UARTSend((uint8_t *) full_buffer, len);
                                        }
                                        xSemaphoreGive(SemBinary);
                                }
                                xSemaphoreGive(SemCountingEmpty);
                        }
                }
                vTaskDelay(xDelay);
        }
}

//*****************************************************************************
//
// Setup serial com, init the semaphores and create the tasks.
//
//*****************************************************************************
int main(void) {
        SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, SYSTEM_CLOCK);
        UARTinit(SYSTEM_CLOCK);
        SemCountingFull = xSemaphoreCreateCounting(BUFFER_SIZE, 0);
        SemCountingEmpty = xSemaphoreCreateCounting(BUFFER_SIZE, BUFFER_SIZE);
        SemBinary = xSemaphoreCreateBinary();
        if (SemBinary != NULL) {
                xSemaphoreGive(SemBinary);
        }
        xTaskCreate( 
                producer,
                "producer",
                200,
                NULL,
                0,
                NULL 
        );
        xTaskCreate( 
                consumer,
                "consumer",
                200,
                NULL,
                0,
                NULL 
        );
        vTaskStartScheduler();
        for (;;) {}
}
