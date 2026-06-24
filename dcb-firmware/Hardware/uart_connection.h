#ifndef __UART_CONNECTION_H__
#define __UART_CONNECTION_H__

#include "main.h"
#include <stdbool.h>

#define UART_RX_TIMEOUT_MS 3000

/* Структура данных очереди UART */
typedef struct
{
    uint8_t data[MAX_UART_DATA_PACKET_SIZE];
    size_t length;
} PacketReceived_t;

/* Структура данных очереди отправки телеметри по UART */
typedef struct
{
    uint8_t bytes[MAX_UART_DATA_PACKET_SIZE]; // пакет телеметрии
    size_t length;                            // длина записанных байт
} TelemetryPacket_t;

void initUARTtoRx();
void idleLineDetectionCallBack(UART_HandleTypeDef *huart);
bool startReadUARTtoBuffer();
uint8_t *getPointToDataFromBuffer(size_t *length);
bool startSendUART(uint8_t *data, size_t length);

#endif