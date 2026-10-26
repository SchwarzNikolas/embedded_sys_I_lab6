#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "task.h"
#include "serial.h"
#include "semphr.h"

#define PHILOSOPHERS 5

static SemaphoreHandle_t forks[PHILOSOPHERS];
static SemaphoreHandle_t uart;
static SemaphoreHandle_t counting_sem;

//*****************************************************************************
//
// Output the action of a certain philosopher to the UART.
//
//*****************************************************************************
static void print_uart(uint32_t id, const char *state) {
        char msg[40];
        snprintf(msg, sizeof(msg), "Philosopher %lu is %s\n", (unsigned long)id, state);
        xSemaphoreTake(uart, portMAX_DELAY);
        UARTSend((uint8_t *)msg, strlen(msg));
        xSemaphoreGive(uart);
}

//*****************************************************************************
//
// Philosopher task which needs two forks to eat.
// Has to aquire a counting semaphore which only lets 4 of them try to eat.
// Then semaphores for both the left and the right fork needs to be aquired.
// After they ate, the release all semaphores.
//
//*****************************************************************************
static void philosopher(void *pvParameters) {
        uint32_t id = (uint32_t)(uintptr_t)pvParameters;
        uint32_t left = id;
        uint32_t right = (id + 1) % PHILOSOPHERS;

        for (;;) {
                print_uart(id, "thinking");
                vTaskDelay(pdMS_TO_TICKS(1000));

                print_uart(id, "hungry");
                xSemaphoreTake(counting_sem, portMAX_DELAY);
                xSemaphoreTake(forks[left],  portMAX_DELAY);
                xSemaphoreTake(forks[right], portMAX_DELAY);

                print_uart(id, "eating");
                vTaskDelay(pdMS_TO_TICKS(1000));

                xSemaphoreGive(forks[right]);
                xSemaphoreGive(forks[left]);
                xSemaphoreGive(counting_sem);
        }
}

//*****************************************************************************
//
// Setup serial com, init the semaphores and create the tasks.
//
//*****************************************************************************
int main(void) {
        SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                        SYSCTL_CFG_VCO_480, SYSTEM_CLOCK);
        UARTinit(SYSTEM_CLOCK);

        uart = xSemaphoreCreateMutex();
        counting_sem = xSemaphoreCreateCounting(PHILOSOPHERS - 1, PHILOSOPHERS - 1);
        uint32_t i;
        for (i = 0; i < PHILOSOPHERS; i++) {
                forks[i] = xSemaphoreCreateMutex();
        }

        for (i = 0; i < PHILOSOPHERS; i++) {
                xTaskCreate(philosopher, "phil", 200,(void *)(uintptr_t)i, 1, NULL);
        }

        vTaskStartScheduler();
        for (;;) {}
}
