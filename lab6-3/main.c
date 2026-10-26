#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "task.h"
#include "serial.h"
#include "queue.h"
#include "driverlib/adc.h"

//*****************************************************************************
//
// Struct to hold all the joystick values
//
//*****************************************************************************
typedef struct {
        uint32_t JoystickX;
        uint32_t JoystickY;
} Joystick;

//*****************************************************************************
//
// Struct to hold all the accelerometer values
//
//*****************************************************************************
typedef struct {
        uint32_t AccelerometerX;
        uint32_t AccelerometerY;
        uint32_t AccelerometerZ;
} Accelerometer;

//*****************************************************************************
//
// Struct to hold all the message queues values
//
//*****************************************************************************
typedef struct{
        QueueHandle_t mic;
        QueueHandle_t joy;
        QueueHandle_t acc;
}Queues;

//*****************************************************************************
//
// Init the ADC component.
//
// Step 0: Joystick Y
// Step 1: Accelerometer Z
// Step 2: Accelerometer Y
// Step 3: Accelerometer X
// Step 4: Mic
// Step 5: Joystick X
//
//*****************************************************************************
void ADCinit(void)
{
        SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
        while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}
        GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
        ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH1);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH3);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH8);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH9 | ADC_CTL_IE | ADC_CTL_END);
        ADCSequenceEnable(ADC0_BASE, 0);
        ADCIntClear(ADC0_BASE, 0);
}

//*****************************************************************************
//
// Read the analog values from the ADC.
//
//*****************************************************************************
void AdcRead(uint32_t samples[])
{
        ADCProcessorTrigger(ADC0_BASE, 0);
        while(!ADCIntStatus(ADC0_BASE, 0, false)) {}
        ADCIntClear(ADC0_BASE, 0);
        ADCSequenceDataGet(ADC0_BASE, 0, samples);
}

//*****************************************************************************
//
// Retrieve the analog values of the accelerometer every 20ms and add to queue.
//
//*****************************************************************************
void Accelerometer_task(void *pvParameters){
        const TickType_t xDelay = 20 / portTICK_PERIOD_MS; // 20 ms delay
        uint32_t samples[6];
        QueueHandle_t xQueueAcc;
        Accelerometer accelerometer;
        xQueueAcc = ( QueueHandle_t ) pvParameters;
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;){
                AdcRead(samples);
                accelerometer.AccelerometerX = samples[3];
                accelerometer.AccelerometerY = samples[2];
                accelerometer.AccelerometerZ = samples[1];
                xQueueSendToBack(xQueueAcc, &accelerometer, 10);
                vTaskDelayUntil(&xLastWakeTime, xDelay);
        }
        
}

//*****************************************************************************
//
// Retrieve the analog values of the joystick every 10ms and add to queue.
//
//*****************************************************************************
void Joystick_task(void *pvParameters){    
        const TickType_t xDelay = 10 / portTICK_PERIOD_MS; // 10 ms delay    
        uint32_t samples[6];
        QueueHandle_t xQueueJoy;
        Joystick joystick;
        xQueueJoy = ( QueueHandle_t ) pvParameters;
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;){
                AdcRead(samples);
                joystick.JoystickY = samples[0];
                joystick.JoystickX = samples[5];
                xQueueSendToBack(xQueueJoy, &joystick, 10);
                vTaskDelayUntil(&xLastWakeTime, xDelay);
        }
}

//*****************************************************************************
//
// Retrieve the analog values of the microphone every 5ms and add to queue.
//
//*****************************************************************************
void microphone_task(void *pvParameters){
        const TickType_t xDelay = 5 / portTICK_PERIOD_MS; // 5 ms delay
        uint32_t samples[6];
        QueueHandle_t xQueueMic;
        xQueueMic = ( QueueHandle_t ) pvParameters;
        uint32_t mic;
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;){
                AdcRead(samples);
                mic = samples[4];
                xQueueSendToBack(xQueueMic, &mic, 10);
                vTaskDelayUntil(&xLastWakeTime, xDelay);
        }
}

//*****************************************************************************
//
// Reads all messages in the queues, calculates average and outputs via UART.
//
//*****************************************************************************
void gatekeeper(void *pvParameters){
        Queues *queues = (Queues *)pvParameters;
        char mic_string[100];
        char joy_string[100];
        char acc_string[100];
        const TickType_t xDelay = 40 / portTICK_PERIOD_MS; // 40 ms delay
        uint8_t messages_mic;
        uint8_t messages_acc;
        uint8_t messages_joy;
        uint32_t avg_mic;
        Joystick avg_joy;
        Accelerometer avg_acc;
        uint32_t new_mic;
        Joystick new_joy;
        Accelerometer new_acc;
        vTaskDelay(xDelay);
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;){
                
                avg_mic = 0;
                avg_joy = (Joystick){0};
                avg_acc = (Accelerometer){0};

                messages_mic = uxQueueMessagesWaiting(queues->mic);
                messages_acc = uxQueueMessagesWaiting(queues->acc);
                messages_joy = uxQueueMessagesWaiting(queues->joy);
                
                if(messages_mic > 0){
                        while (xQueueReceive(queues->mic, &new_mic, 0) == pdPASS) {
                                avg_mic += new_mic;
                        }
                        avg_mic = (avg_mic / messages_mic)-2000;
                        if (avg_mic > 200){
                                avg_mic = 0;
                        }
                }
                if(messages_joy > 0){
                        while (xQueueReceive(queues->joy, &new_joy, 0) == pdPASS) {
                                avg_joy.JoystickX += new_joy.JoystickX;
                                avg_joy.JoystickY += new_joy.JoystickY;
                        }
                        avg_joy.JoystickX = avg_joy.JoystickX / messages_joy;
                        avg_joy.JoystickY = avg_joy.JoystickY / messages_joy;
                }
                if(messages_acc > 0){
                        while (xQueueReceive(queues->acc, &new_acc, 0) == pdPASS) {
                                avg_acc.AccelerometerX += new_acc.AccelerometerX;
                                avg_acc.AccelerometerY += new_acc.AccelerometerY;
                                avg_acc.AccelerometerZ += new_acc.AccelerometerZ;
                        }
                        avg_acc.AccelerometerX = avg_acc.AccelerometerX / messages_acc;
                        avg_acc.AccelerometerY = avg_acc.AccelerometerY / messages_acc;
                        avg_acc.AccelerometerZ = avg_acc.AccelerometerZ / messages_acc;
                }
                
                snprintf(mic_string, sizeof(mic_string),"Microphone: %ludB", (unsigned long)avg_mic);
                snprintf(joy_string, sizeof(joy_string),"\nJoystick X: %lu, Y: %lu", (unsigned long)avg_joy.JoystickX, (unsigned long)avg_joy.JoystickY);
                snprintf(acc_string, sizeof(acc_string),"\nAccelerometer X: %lu, Y: %lu, Z: %lu", (unsigned long)avg_acc.AccelerometerX,
                        (unsigned long)avg_acc.AccelerometerY,(unsigned long)avg_acc.AccelerometerZ);
                UARTSend((uint8_t *)"\e[2J", 4);
                UARTSend((uint8_t *)"\e[H", 3);   
                UARTSend((uint8_t *)mic_string, strlen(mic_string));
                UARTSend((uint8_t *)joy_string, strlen(joy_string));
                UARTSend((uint8_t *)acc_string, strlen(acc_string));          
                UARTSend((uint8_t *)"\r\n", 2);
                vTaskDelayUntil(&xLastWakeTime, xDelay);
                
        }
}

//*****************************************************************************
//
// Setup the queues, init the adc and serial com and create needed tasks.
//
//*****************************************************************************
int main(void) {
        SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, SYSTEM_CLOCK);

        // Messages queues that should hold the sensor readings
        QueueHandle_t xQueueMic, xQueueJoy, xQueueAcc;

        xQueueAcc = xQueueCreate(2, sizeof(Accelerometer));
        xQueueJoy = xQueueCreate(4, sizeof(Joystick));
        xQueueMic = xQueueCreate(8, sizeof(uint32_t));
        
        static Queues queues;
        queues = (Queues){xQueueMic, xQueueJoy, xQueueAcc};

        ADCinit();
        UARTinit(SYSTEM_CLOCK);

        xTaskCreate( 
                microphone_task,
                "Zeus",
                512,
                (void *) xQueueMic,
                0,
                NULL 
        );
        xTaskCreate( 
                Accelerometer_task,
                "Hermes",
                512,
                (void *) xQueueAcc,
                0,
                NULL 
        );
        xTaskCreate( 
                Joystick_task,
                "Aphrodite",
                512,
                (void *) xQueueJoy,
                0,
                NULL 
        );
        xTaskCreate( 
                gatekeeper,
                "Cerebus",
                1024,
                &queues,
                1,
                NULL 
        );
        vTaskStartScheduler();
        for (;;) {
        }
}
