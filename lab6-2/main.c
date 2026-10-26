#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "task.h"
#include "buttons.h"
#include "serial.h"

char counter_string[100];

//*****************************************************************************
//
// Show the number of inputted characters for 10s after a button press.
//
//*****************************************************************************
static void Button_Press(void *pvParameters) {
        const TickType_t xDelay = 50 / portTICK_PERIOD_MS; // 50 ms delay
        uint32_t timestamp_ms_left = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t timestamp_ms_right = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t current_timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        unsigned char ucDelta, ucState;
        bool show_secret_text_that_is_only_allowed_to_be_shown_for_10s = false;
        for (;;){
                ucState = ButtonsPoll(&ucDelta, 0);
                if (show_secret_text_that_is_only_allowed_to_be_shown_for_10s) {
                        snprintf(counter_string, sizeof(counter_string),"\nTotal amount of characters inputted: %lu", (unsigned long)counter);
                }
                if (BUTTON_PRESSED(LEFT_BUTTON, ucState, ucDelta)) {
                        timestamp_ms_left = xTaskGetTickCount() * portTICK_PERIOD_MS;
                        show_secret_text_that_is_only_allowed_to_be_shown_for_10s = true;
                        
                }
                current_timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
                if ((current_timestamp - timestamp_ms_left) >= 10000) {
                        show_secret_text_that_is_only_allowed_to_be_shown_for_10s = false;
                        counter_string[0] = '\0';
                }
                vTaskDelay(xDelay);
        }      
}

//*****************************************************************************
//
// Output the last 15 characters recieved by the UART back to the serial com.
// Also shows the amount of inputted chars if button has been activated.
//
//*****************************************************************************
void echo_echo(void *pvParameters){
        const TickType_t xDelay = 50 / portTICK_PERIOD_MS; // 50 ms delay
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;){
                vTaskDelayUntil(&xLastWakeTime, xDelay);
                UARTSend((uint8_t *)"\e[2J", 4);
                UARTSend((uint8_t *)"\e[H", 3);
                UARTSend((uint8_t *)"\x1B[?25l", 6);
                if (counter < 1){
                        continue;
                }
                UARTSend((uint8_t *) input, strlen(input));
                if (strlen(counter_string) > 0){
                        UARTSend((uint8_t *) counter_string, strlen(counter_string));
                }
                UARTSend((uint8_t *)"\r\n", 2);
        }
}

//*****************************************************************************
//
// Setup button, serial com and create needed tasks.
//
//*****************************************************************************
int main(void) {
        SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                SYSCTL_CFG_VCO_480, SYSTEM_CLOCK);
        ButtonsInit();
        UARTinit(SYSTEM_CLOCK);
        
        xTaskCreate( 
                echo_echo,
                "echo",
                200,
                NULL,
                0,
                NULL 
        );
        xTaskCreate( 
                Button_Press,
                "ButtonPolling",
                200,
                NULL,
                1,
                NULL 
        );
        vTaskStartScheduler();
        for (;;) {
        }
}
