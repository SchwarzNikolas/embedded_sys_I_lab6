#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

extern char input[16];
extern uint8_t index;
extern uint32_t counter;

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count);
void UARTIntHandler(void);
void UARTinit(uint32_t g_ui32SysClock);

#endif
