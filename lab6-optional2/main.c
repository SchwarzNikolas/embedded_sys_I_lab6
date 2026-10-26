#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "task.h"
#include "buttons.h"
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
        const TickType_t xDelay = pdMS_TO_TICKS(1000); // 100 ms delay
        vTaskDelay(xDelay);

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
        const TickType_t xDelay = pdMS_TO_TICKS(1000); // 100ms delay
        vTaskDelay(xDelay);

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
// Create a random amount of producer and consumer tasks on button press.
// The button press seeds the random function with the tick count of the inital
// press, then a random number of producers and consumers are created.
//
//*****************************************************************************
void Button_Press(void *pvParameters){
        const TickType_t xDelay = 50 / portTICK_PERIOD_MS;
        unsigned char ucDelta, ucState;

        for (;;) {
                ucState = ButtonsPoll(&ucDelta, 0);

                if (BUTTON_PRESSED(LEFT_BUTTON, ucState, ucDelta)) {
                        srand((unsigned int)xTaskGetTickCount());

                        uint32_t conint = (rand() % 11) + 1;
                        uint32_t prodint = (rand() % 11) + 1;

                        char producers[3];
                        char consumers[3];
                        producers[0] = '0' + prodint / 10;
                        producers[1] = '0' + prodint % 10;
                        producers[2] = '\0';
                        consumers[0] = '0' + conint / 10;
                        consumers[1] = '0' + conint % 10;
                        consumers[2] = '\0';
                        UARTSend("Producers: ", strlen("Producers: "));
                        UARTSend((uint8_t *)producers, strlen(producers));
                        UARTSend(" Consumers: ", strlen(" Consumers: "));
                        UARTSend((uint8_t *)consumers, strlen(consumers));
                        UARTSend("\n", 1);    

                        uint32_t i;

                        for(i = 0; i < conint; i++){
                                xTaskCreate( 
                                        consumer,
                                        "consumer_name",
                                        200,
                                        NULL,
                                        0,
                                        NULL 
                                        ); 
                        }
                        
                        for(i = 0; i < prodint; i++){
                                xTaskCreate( 
                                        producer,
                                        "producer_name",
                                        200,
                                        NULL,
                                        0,
                                        NULL 
                                );
                        }

                        vTaskDelete(NULL);
                }

                vTaskDelay(xDelay);
        }
}

//*****************************************************************************
//
// Setup serial com, init the semaphores and create the task which spawn the
// consumer and producer tasks.
//
//*****************************************************************************
int main(void) {
        SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, SYSTEM_CLOCK);
        UARTinit(SYSTEM_CLOCK);
        ButtonsInit();
        SemCountingFull = xSemaphoreCreateCounting(BUFFER_SIZE, 0);
        SemCountingEmpty = xSemaphoreCreateCounting(BUFFER_SIZE, BUFFER_SIZE);
        SemBinary = xSemaphoreCreateBinary();
        if (SemBinary != NULL) {
                xSemaphoreGive(SemBinary);
        }

        xTaskCreate(
                Button_Press,
                "button",
                512,
                NULL,
                0,
                NULL
        );
    
        vTaskStartScheduler();
        for (;;) {}
}
