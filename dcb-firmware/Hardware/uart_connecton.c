#include "uart_connection.h"
#include "usart.h"
#include "cmsis_os.h"

#define USE_UART huart2
extern SemaphoreHandle_t xUART_RxCpltSemaphore;
extern SemaphoreHandle_t xUART_TxCpltSemaphore;

// Буфер для DMA
#define DMA_RX_BUFFER_SIZE MAX_UART_DATA_PACKET_SIZE
struct
{
    uint8_t buffer[DMA_RX_BUFFER_SIZE];
    size_t length; // длина записанных байт в буфер
} dma_rx;

/**
 * @brief Настройка UART на прием данных.
 * Включение прерывания IDLE Line Detection. Запуск приема по DMA
 */
void initUARTtoRx()
{
    /* Включаем IDLE Line Detection */
    __HAL_UART_ENABLE_IT(&USE_UART, UART_IT_IDLE);
}

/**
 * @brief Чтение UART через DMA. Сохранение принятых байт в буфер
 * @return true в случае успеха
 */
bool startReadUARTtoBuffer()
{
    /* Запуск чтения байт (до Idle Line или до максимального значения приемного буфера) */
    if (HAL_UART_Receive_DMA(&USE_UART, dma_rx.buffer, DMA_RX_BUFFER_SIZE) == HAL_OK)
        return true;
    else
        return false;
}

/**
 * @brief Отправка UART через DMA
 * @param data указатель на данные
 * @param length длина данных
 * @return true в случае успеха
 */
bool startSendUART(uint8_t *data, size_t length)
{
    if (HAL_UART_Transmit_DMA(&USE_UART, data, length) == HAL_OK)
        return true;
    else
        return false;
}

/**
 * @brief Выдача указателя на начало блока данных и длину в буфере.
 * @param length запишется длина блока данных
 * @return указатель на начало блока. NULL в случае ошибки
 */
uint8_t *getPointToDataFromBuffer(size_t *length)
{
    if (dma_rx.length == 0 || dma_rx.length > DMA_RX_BUFFER_SIZE)
        return NULL;
    *length = dma_rx.length;
    return dma_rx.buffer;
}

/**
 * @brief Обработка окончания приема пакета данных по UART (линия в состоянии IDLE).
 * Вызывается в прерывании по IDLE
 */
void idleLineDetectionCallBack(UART_HandleTypeDef *huart)
{
    dma_rx.length = huart->RxXferSize - huart->hdmarx->Instance->CNDTR; // сохраняем кол-во записанных байт в буфер

    /**
     * IDLE Line может сработать при запуске МК,
     * даже без приема UART (например, при перезагрузке).
     * Поэтоому необходимо проверить, пришли ли какие-либо данные.
     */
    if (dma_rx.length == 0)
        return;

    HAL_UART_DMAStop(huart); // останавливаем DMA

    /**
     * Разблокируем семафор чтобы задача UART завершила приема данных
     * Момент окончания приема пакета считается состояние свободной линии UART - Idle Line
     */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xUART_RxCpltSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief callback по завершению приема UART через DMA.
 * Вызывается коггда считаны все байты в буфер указанного размера DMA_RX_BUFFER_SIZE
 * (на тот случай, когда Idle Line Detection не сработает)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USE_UART.Instance)
    {
        dma_rx.length = huart->RxXferSize;

        /* Вызываем обработку пакета CMDTask */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xUART_RxCpltSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USE_UART.Instance)
    {
        /* Разблокируем задачу отправки данных по UART*/
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xUART_TxCpltSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USE_UART.Instance)
    {
        /* Разблокируем задачу отправки данных по UART*/
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xUART_TxCpltSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}