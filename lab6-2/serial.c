#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "time.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

char input[16];
uint8_t index = 0;
uint32_t counter = 0;

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        MAP_UARTCharPut(UART0_BASE, *pui8Buffer++);
    }
}


//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void UARTIntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = MAP_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    MAP_UARTIntClear(UART0_BASE, ui32Status);

    while(MAP_UARTCharsAvail(UART0_BASE)){
        //
        // Read the next character from the UART and write it back to the UART.
        //
        char c = MAP_UARTCharGetNonBlocking(UART0_BASE);


        // append the character to the string array
        if (index < 15){
            input[index++] = c;
            input[index] = '\0';
        } 
        else{
            uint8_t i;
            for(i = 0; i < 14; i++){
                input[i] = input[i+1];
            }
            input[14] = c;
            input[15] = '\0';
        }

        counter++;
    }
}



/*
#define BUFFER_SIZE 15

char buffer[BUFFER_SIZE + 1] = "ABCDEFGHIJKLMNO";

void AppendChar(char newChar)
{
    for (int i = 0; i < BUFFER_SIZE - 1; i++)
    {
        buffer[i] = buffer[i + 1];
    }

    buffer[BUFFER_SIZE - 1] = newChar;
    buffer[BUFFER_SIZE] = '\0';
}
*/

//*****************************************************************************
//
// Init the UART component.
//
//*****************************************************************************
void UARTinit(uint32_t g_ui32SysClock){
        //
    // Enable the GPIO port that is used for the on-board LED.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Enable the GPIO pins for the LED (PN0).
    //
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    //
    // Enable the peripherals used by this example.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable processor interrupts.
    //
    MAP_IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    MAP_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    MAP_IntEnable(INT_UART0);
    MAP_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}
